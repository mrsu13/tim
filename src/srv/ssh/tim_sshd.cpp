#include "tim_ssh_server.h"

#include "tim_application.h"
#include "tim_prefs.h"
#include "tim_tools.h"

#include <co_areactor.h>
#include <co_at_scope_exit.h>
#include <co_file.h>
#include <co_security_tools.h>


// Public Methods

tim::ssh_server::ssh_server(std::uint16_t port, co::object *parent)
    : co::io_server(parent)

    // Signal
    , new_session(this)

    , _port(port)
    , _bind()
    , _sessions()
{
    CO_TRY_IN_CTOR(init());
}

tim::ssh_server::~ssh_server()
{
    ssh_bind_free(_bind);
    ssh_finalize();
}

bool tim::ssh_server::has_session(const std::string &imei) const
{
    CO_ASSERT(!imei.empty());

    for (const session_list::value_type &i: _sessions)
        if (i->imei() == imei)
            return true;
    return false;
}

co::rc tim::ssh_server::dispatch()
{
    session_list::iterator i = _sessions.begin();
    while (i != _sessions.end())
        if ((*i)->dispatch())
            ++i;
        else
            i = _sessions.erase(i);

    return co::Ok;
}


// Private Methods

co::rc tim::ssh_server::init()
{
    assert(_port && "port must be positive.");

    if (!co::become_root())
        return CO_ERROR("%s", "Failed to become root.");

    CO_AT_SCOPE_EXIT(
        if (!co::become_real_user())
            CO_ERROR("%s", "Failed to become the ordinary user.");
    );

    SSH_TRY(ssh_init(), "Failed to initialize SSH.", nullptr);

    if (!(_bind = ssh_bind_new()))
        return CO_ERROR("%s", "Failed to allocate SSH bind.");

/*
    SSH_TRY(ssh_bind_options_set(_bind, SSH_BIND_OPTIONS_DSAKEY,
                                 co::file::complete_path((tim::SSH_KEYS_BASE_PATH
                                                                / "ssh_host_dsa_key")
                                                         .string()).c_str()),
             "Failed to set DSA key path", _bind);
*/

    SSH_TRY(ssh_bind_options_set(_bind, SSH_BIND_OPTIONS_RSAKEY,
                                 co::file::complete_path((tim::SSH_KEYS_BASE_PATH
                                                                / "ssh_host_rsa_key")
                                                         .string()).c_str()),
            "Failed to set RSA key path", _bind);

    SSH_TRY(ssh_bind_options_set(_bind, SSH_BIND_OPTIONS_BINDPORT, &_port),
            "Failed to set RSSH port", _bind);

#ifdef TIM_DEBUG
    SSH_TRY(ssh_bind_options_set(_bind, SSH_BIND_OPTIONS_LOG_VERBOSITY_STR, "3"),
            "Failed to set verbose mode", _bind);
#else
    SSH_TRY(ssh_bind_options_set(_bind, SSH_BIND_OPTIONS_LOG_VERBOSITY_STR, "0"),
            "Failed to set verbose mode", _bind);
#endif

    SSH_TRY(ssh_bind_listen(_bind),
            "Failed to listen on SSH port", _bind);

    set_descriptor(ssh_bind_get_fd(_bind));

    CO_CONNECT(this, new_connection, this, tim::ssh_server::on_new_session);

    tim::app()->reactor()->add(this);

    CO_INFO("SSH server started at port %u.", _port);

    return co::Ok;
}

co::rc tim::ssh_server::on_new_session()
{
    CO_DEBUG("New SSH session at port %u.", _port);

    ::ssh_session s = ssh_new();
    if (!s)
        return CO_ERROR("%s", "Failed to allocate SSH session.");

    CO_AT_SCOPE_EXIT(
        if (s)
            ssh_free(s);
    );

    SSH_TRY(ssh_bind_accept(_bind, s),
            "Failed to accept incoming connection", _bind);

    std::unique_ptr<tim::ssh_session> session(new tim::ssh_session(s, this));
    if (!session->valid())
        return CO_ERROR("%s", "Failed to create SSH session.");

    CO_TRY(tim::app()->reactor()->add(session.get()));

    CO_CONNECT(session.get(), authenticated,
               this, tim::ssh_server::on_session_authenticated);

    _sessions.emplace_back(std::move(session));

    s = nullptr;

    return co::Ok;
}

void tim::ssh_server::on_session_authenticated()
{
    new_session.fire(static_cast<tim::ssh_session *>(emitter()));
}
