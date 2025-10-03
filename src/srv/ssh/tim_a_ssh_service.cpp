#include "tim_a_ssh_service.h"

#include "tim_application.h"
#include "tim_prefs.h"
#include "tim_ssh_server.h"
#include "tim_tools.h"

#include <co_areactor.h>

#include <libssh/server.h>

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <utmp.h>


static const std::size_t BUF_SIZE = 1048576;

// Public Methods

tim::a_ssh_service::a_ssh_service(::a_ssh_service session,
                               tim::ssh_server *server)
    : co::io_descriptor(ssh_get_fd(session), server)

    // Signals
    , authenticated(this)
    , ready_read(this)

    , _server(server)
    , _session(session)
    , _channel(nullptr)
    , _event()
    , _winsize()
    , _auth_attempts(0)
    , _authenticated(false)
    , _server_cb()
    , _channel_cb()
    , _imei()
    , _incoming_data()
    , _outgoing_data()
    , _is_running(true)
{
    CO_ASSERT(_server);
    CO_ASSERT(_session);

    CO_TRY_IN_CTOR(init());

    CO_DEBUG("New session created.");
}

tim::a_ssh_service::~a_ssh_service()
{
    ssh_channel_send_eof(_channel);
    ssh_channel_close(_channel);
    ssh_channel_free(_channel);

    ssh_event_free(_event);

    ssh_disconnect(_session);
    ssh_free(_session);

    CO_INFO("SSH session to '%s' destroyed.", _imei.c_str());
}

std::size_t tim::a_ssh_service::send_data(std::byte_vector &data)
{
    if (data.empty())
        return 0;

    const std::size_t sz = data.size();
    const std::size_t i = _outgoing_data.size();
    _outgoing_data.resize(i + sz);
    std::memcpy(&_outgoing_data[i], &data[0], sz);
    data.clear();

    return sz;
}

std::size_t tim::a_ssh_service::send_string(const std::string &s)
{
    if (s.empty())
        return 0;

    std::byte_vector data(s.size());
    std::memcpy(&data[0], &s[0], s.size());
    return send_data(data);
}

std::size_t tim::a_ssh_service::take_data(std::byte_vector &data)
{
    if (_incoming_data.empty())
        return 0;

    const std::size_t sz = _incoming_data.size();
    const std::size_t i = data.size();
    data.resize(i + sz);
    std::memcpy(&data[i], &_incoming_data[0], sz);
    _incoming_data.clear();

    return sz;
}

co::rc tim::a_ssh_service::flush()
{
    if (_outgoing_data.empty())
        return co::Ok;

    if (ssh_channel_write(_channel, _outgoing_data.data(), _outgoing_data.size()) == SSH_ERROR)
        return CO_ERROR("Failed to flush outgoing data: %s",
                        ssh_get_error(_channel));

    _outgoing_data.clear();

    return co::Ok;
}

bool tim::a_ssh_service::dispatch()
{
    if (!_is_running)
        return false;

    if (!_authenticated
            || !_channel)
    {
        /* If the user has used up all attempts, or if he hasn't been able to
         * authenticate in 10 seconds (n * 100ms), disconnect. */
        if (_auth_attempts >= 3)
            return false;

        return ssh_event_dopoll(_event, 0) != SSH_ERROR;
    }

    /* Poll the main event which takes care of the session, the channel and
     * even our child process's stdout/stderr (once it's started). */
    if (ssh_event_dopoll(_event, 0) == SSH_ERROR)
        return false;

    return ssh_channel_is_open(_channel);
}


// Private Methods

co::rc tim::a_ssh_service::init()
{
    CO_DEBUG("New SSH session established.");

    _server_cb.userdata = this;
    _server_cb.auth_password_function = &tim::a_ssh_service::auth_password;
    _server_cb.channel_open_request_session_function = &tim::a_ssh_service::channel_open;

    ssh_callbacks_init(&_server_cb);

    _channel_cb.userdata = this,

    _channel_cb.channel_pty_request_function = &tim::a_ssh_service::pty_request;
    _channel_cb.channel_pty_window_change_function = &tim::a_ssh_service::pty_resize;
    _channel_cb.channel_shell_request_function = &tim::a_ssh_service::shell_request;
//    _channel_cb.channel_exec_request_function = &tim::a_ssh_service::exec_request;
    _channel_cb.channel_data_function = &tim::a_ssh_service::data_function;
//    _channel_cb.channel_subsystem_request_function = &tim::a_ssh_service::subsystem_request;

    ssh_callbacks_init(&_channel_cb);

    ssh_set_auth_methods(_session, SSH_AUTH_METHOD_PASSWORD);

    _event = ssh_event_new();

    SSH_TRY(ssh_set_server_callbacks(_session, &_server_cb),
            "Failed to set server call-backs", _session);

    SSH_TRY(ssh_handle_key_exchange(_session),
            "Key exchange failed", _session);

    ssh_set_blocking(_session, 0); // Non-blocking

    SSH_TRY(ssh_event_add_session(_event, _session),
            "Failed to add session to event", _event);

    return co::Ok;
}

int tim::a_ssh_service::data_function(::a_ssh_service session, ::ssh_channel channel, void *data,
                                     std::uint32_t len, int is_stderr, void *userdata)
{
    (void) session;
    (void) channel;
    (void) is_stderr;

    if (len == 0)
        return 0;

    tim::a_ssh_service *self = static_cast<tim::a_ssh_service *>(userdata);
    CO_ASSERT(self);

    const std::size_t cur_size = self->_incoming_data.size();
    self->_incoming_data.resize(cur_size + len);
    std::memcpy(&self->_incoming_data[cur_size], (const std::uint8_t *)data, len);

    self->ready_read.fire();

// Echo
//    ssh_channel_write(self->_channel, (const char *)data, len);

    return len;
}


int tim::a_ssh_service::pty_request(::a_ssh_service session, ::ssh_channel channel,
                                   const char *term, int cols, int rows, int py, int px,
                                   void *userdata)
{
    (void) session;
    (void) channel;
    (void) term;

    tim::a_ssh_service *self = static_cast<tim::a_ssh_service *>(userdata);
    CO_ASSERT(self);

    self->_winsize.ws_row = rows;
    self->_winsize.ws_col = cols;
    self->_winsize.ws_xpixel = px;
    self->_winsize.ws_ypixel = py;

    return SSH_OK;
}

int tim::a_ssh_service::pty_resize(::a_ssh_service session, ::ssh_channel channel,
                                   int cols, int rows, int py, int px, void *userdata)
{
    (void) session;
    (void) channel;

    tim::a_ssh_service *self = static_cast<tim::a_ssh_service *>(userdata);
    CO_ASSERT(self);

    self->_winsize.ws_row = rows;
    self->_winsize.ws_col = cols;
    self->_winsize.ws_xpixel = px;
    self->_winsize.ws_ypixel = py;

    return SSH_OK;
}

int tim::a_ssh_service::shell_request(::a_ssh_service session, ::ssh_channel channel,
                                      void *userdata)
{
    (void) session;
    (void) channel;

    tim::a_ssh_service *self = static_cast<tim::a_ssh_service *>(userdata);
    CO_ASSERT(self);

    return SSH_OK;
}

int tim::a_ssh_service::auth_password(::a_ssh_service session, const char *user,
                                      const char *pass, void *userdata)
{
    (void) session;

    tim::a_ssh_service *self = static_cast<tim::a_ssh_service *>(userdata);
    CO_ASSERT(self);

    const std::string imei = imei_by_user_password(user, pass);
    if (!imei.empty())
    {
        CO_DEBUG("SSH session for device with IMEI '%s'.", imei.c_str());
        if (self->_server->has_session(imei))
            CO_ERROR("Duplicated SSH session for IMEI '%s'.", imei.c_str());
        else
        {
            self->_imei = imei;
            if (!self->_imei.empty())
            {
                self->_authenticated = true;
                self->authenticated.fire();
                return SSH_AUTH_SUCCESS;
            }
        }
    }

    ++(self->_auth_attempts);

    return SSH_AUTH_DENIED;
}

int tim::a_ssh_service::auth_publickey(::a_ssh_service session,
                                       const char *user,
                                       struct ssh_key_struct *pubkey,
                                       char signature_state,
                                       void *userdata)
{
    (void) user;
    (void) session;
    (void) pubkey;

    if (signature_state == SSH_PUBLICKEY_STATE_NONE)
        return SSH_AUTH_SUCCESS;

    if (signature_state != SSH_PUBLICKEY_STATE_VALID)
        return SSH_AUTH_DENIED;

    tim::a_ssh_service *self = static_cast<tim::a_ssh_service *>(userdata);
    CO_ASSERT(self);
/*
    // Valid so far.  Now look through authorized keys for a match
    if (!authorizedkeys.empty())
    {
        ssh_key key = nullptr;
        int res;
        struct stat buf;

        if (::stat(authorizedkeys, &buf) == 0)
        {
            if ((res = ssh_pki_import_pubkey_file(authorizedkeys, &key)) != SSH_OK
                    || !key)
                CO_ERROR("Unable to import public key file '%s'.",
                         authorizedkeys);
            else
            {
                res = ssh_key_cmp(key, pubkey, SSH_KEY_CMP_PUBLIC);
                ssh_key_free(key);
                if (res == SSH_OK)
                {
                    self->_authenticated = true;
                    return SSH_AUTH_SUCCESS;
                }
            }
        }
    }
*/
    // No matches.
    self->_authenticated = false;
    return SSH_AUTH_DENIED;
}

::ssh_channel tim::a_ssh_service::channel_open(::a_ssh_service session, void *userdata)
{
    tim::a_ssh_service *self = static_cast<tim::a_ssh_service *>(userdata);
    CO_ASSERT(self);

    self->_channel = ssh_channel_new(session);
    ssh_set_channel_callbacks(self->_channel, &self->_channel_cb);

    return self->_channel;
}

std::string tim::a_ssh_service::imei_by_user_password(const std::string &user,
                                                     const std::string &password)
{
    for (const tim::device_map::value_type &pair: tim::app()->config().devices())
        if (pair.second.name() == user
                && pair.second.password() == password)
            return pair.first;

    return std::string();
}
