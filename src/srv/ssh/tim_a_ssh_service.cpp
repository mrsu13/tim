#include "tim_a_ssh_service.h"

#include "tim_a_ssh_service_p.h"

#include <libssh/server.h>

#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <utmp.h>


static const std::size_t BUF_SIZE = 1048576;

// Public

tim::a_ssh_service::a_ssh_service(const std::string &name, ssh_session session)
    : tim::a_io_device(ssh_get_fd(session))
    , tim::service(name)

    // Signals
    , authenticated()

    , _d(new tim::p::a_ssh_service())
{
    assert(session);

    _d->_session = session;
    _d->_running = true;

    _d->_server_cb.userdata = _d.get();
    _d->_server_cb.auth_password_function = &tim::p::a_ssh_service::auth_password;
    _d->_server_cb.channel_open_request_session_function = &tim::p::a_ssh_service::channel_open;

    ssh_callbacks_init(&_server_cb);

    _d->_channel_cb.userdata = _d.get();

    _d->_channel_cb.channel_pty_request_function = &tim::p::a_ssh_service::pty_request;
    _d->_channel_cb.channel_pty_window_change_function = &tim::p::a_ssh_service::pty_resize;
    _d->_channel_cb.channel_shell_request_function = &tim::p::a_ssh_service::shell_request;
//    _d->_channel_cb.channel_exec_request_function = &tim::p::a_ssh_service::exec_request;
    _d->_channel_cb.channel_data_function = &tim::p::a_ssh_service::data_function;
//    _d->_channel_cb.channel_subsystem_request_function = &tim::p::a_ssh_service::subsystem_request;

    ssh_callbacks_init(&_d->_channel_cb);

    ssh_set_auth_methods(_d->_session, SSH_AUTH_METHOD_PASSWORD);

    _d->_event = ssh_event_new();

    SSH_TRY(ssh_set_server_callbacks(_d->_session, &_d->_server_cb),
            "Failed to set server call-backs", _d->_session);

    SSH_TRY(ssh_handle_key_exchange(_d->_session),
            "Key exchange failed", _d->_session);

    ssh_set_blocking(_d->_session, 0); // Non-blocking

    SSH_TRY(ssh_event_add_session(_d->_event, _d->_session),
            "Failed to add session to event", _d->_event);
}

tim::a_ssh_service::~a_ssh_service()
{
    ssh_channel_send_eof(_d->_channel);
    ssh_channel_close(_d->_channel);
    ssh_channel_free(_d->_channel);

    ssh_event_free(_d->_event);

    ssh_disconnect(_d->_session);
    ssh_free(_d->_session);
}

tim::byte_vector &tim::a_ssh_service::outgoing_data()
{
    return d->_outgoing_data;
}

std::size_t tim::a_ssh_service::send_data(tim::byte_vector &&data)
{
    if (data.empty())
        return 0;

    const std::size_t sz = data.size();
    const std::size_t i = _d->_outgoing_data.size();
    _d->_outgoing_data.resize(i + sz);
    std::memcpy(&_d->_outgoing_data[i], &data[0], sz);
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

std::size_t tim::a_ssh_service::data_size() const
{
    return _d->_incoming_data.size();
}

std::size_t tim::a_ssh_service::take_data(std::byte_vector &data)
{
    if (_d->_incoming_data.empty())
        return 0;

    const std::size_t sz = _d->_incoming_data.size();
    const std::size_t i = data.size();
    data.resize(i + sz);
    std::memcpy(&data[i], &_d->_incoming_data[0], sz);
    _d->_incoming_data.clear();

    return sz;
}

bool tim::a_ssh_service::flush()
{
    if (_d->_outgoing_data.empty())
        return true;

    if (ssh_channel_write(_d->_channel, _d->_outgoing_data.data(), _d->_outgoing_data.size()) == SSH_ERROR)
        return TIM_TRACE(Error,
                         TIM_TR("Failed to flush outgoing data: %s"_en,
                                "Ошибка при отправке данных: %s"_ru),
                         ssh_get_error(_channel));

    _d->_outgoing_data.clear();

    return true;
}

bool tim::a_ssh_service::dispatch()
{
    if (!_d->_running)
        return false;

    if (!_d->_authenticated
            || !_d->_channel)
    {
        /* If the user has used up all attempts, or if he hasn't been able to
         * authenticate in 10 seconds (n * 100ms), disconnect. */
        if (_d->_auth_attempts >= 3)
            return false;

        return ssh_event_dopoll(_d->_event, 0) != SSH_ERROR;
    }

    /* Poll the main event which takes care of the session, the channel and
     * even our child process's stdout/stderr (once it's started). */
    if (ssh_event_dopoll(_d->_event, 0) == SSH_ERROR)
        return false;

    return ssh_channel_is_open(_d->_channel);
}

bool tim::a_ssh_service::running() const
{
    return _d->_running;
}

void tim::a_ssh_service::terminate()
{
    _d->_running = false;
}

// Private

int tim::p::a_ssh_service::data_function(ssh_session session, ssh_channel channel, void *data,
                                         std::uint32_t len, int is_stderr, void *user_data)
{
    (void) session;
    (void) channel;
    (void) is_stderr;

    if (len == 0)
        return 0;

    tim::p::a_ssh_service *self = (tim::p::a_ssh_service *)user_data;
    assert(self);

    const std::size_t cur_size = self->_incoming_data.size();
    self->_incoming_data.resize(cur_size + len);
    std::memcpy(&self->_incoming_data[cur_size], (const std::uint8_t *)data, len);

    self->_q->ready_read();

// Echo
//    ssh_channel_write(self->_channel, (const char *)data, len);

    return len;
}

int tim::p::a_ssh_service::pty_request(ssh_session session, ssh_channel channel,
                                       const char *term, int cols, int rows, int py, int px,
                                       void *user_data)
{
    (void) session;
    (void) channel;
    (void) term;

    tim::p::a_ssh_service *self = (tim::p::a_ssh_service *)user_data;
    assert(self);

    self->_winsize.ws_row = rows;
    self->_winsize.ws_col = cols;
    self->_winsize.ws_xpixel = px;
    self->_winsize.ws_ypixel = py;

    return SSH_OK;
}

int tim::p::a_ssh_service::pty_resize(ssh_session session, ssh_channel channel,
                                      int cols, int rows, int py, int px, void *user_data)
{
    (void) session;
    (void) channel;

    tim::p::a_ssh_service *self = (tim::p::a_ssh_service *)user_data;
    assert(self);

    self->_winsize.ws_row = rows;
    self->_winsize.ws_col = cols;
    self->_winsize.ws_xpixel = px;
    self->_winsize.ws_ypixel = py;

    return SSH_OK;
}

int tim::p::a_ssh_service::shell_request(ssh_session session, ssh_channel channel,
                                         void *user_data)
{
    (void) session;
    (void) channel;

    tim::p::a_ssh_service *self = (tim::p::a_ssh_service *)user_data;
    assert(self);

    return SSH_OK;
}

int tim::p::a_ssh_service::auth_password(ssh_session session, const char *user,
                                         const char *pass, void *user_data)
{
    (void) session;

    tim::p::a_ssh_service *self = (tim::p::a_ssh_service *)user_data;
    assert(self);

    self->_authenticated = true;
    self->_q->authenticated();

    return SSH_AUTH_SUCCESS;

/*
    ++(self->_auth_attempts);

    return SSH_AUTH_DENIED;
*/
}

int tim::p::a_ssh_service::auth_publickey(ssh_session session,
                                          const char *user,
                                          struct ssh_key_struct *pubkey,
                                          char signature_state,
                                          void *user_data)
{
    (void) user;
    (void) session;
    (void) pubkey;

    if (signature_state == SSH_PUBLICKEY_STATE_NONE)
        return SSH_AUTH_SUCCESS;

    if (signature_state != SSH_PUBLICKEY_STATE_VALID)
        return SSH_AUTH_DENIED;

    tim::p::a_ssh_service *self = (tim::p::a_ssh_service *)user_data;
    assert(self);
/*
    // Valid so far. Now look through authorized keys for a match
    if (!authorizedkeys.empty())
    {
        ssh_key key = nullptr;
        int res;
        struct stat buf;

        if (::stat(authorizedkeys, &buf) == 0)
        {
            if ((res = ssh_pki_import_pubkey_file(authorizedkeys, &key)) != SSH_OK
                    || !key)
                TIM_TRACE(Error, "Unable to import public key file '%s'.",
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

ssh_channel tim::p::a_ssh_service::channel_open(ssh_session session, void *user_data)
{
    tim::p::a_ssh_service *self = (tim::p::a_ssh_service *)user_data;
    assert(self);

    self->_channel = ssh_channel_new(session);
    ssh_set_channel_callbacks(self->_channel, &self->_channel_cb);

    return self->_channel;
}
