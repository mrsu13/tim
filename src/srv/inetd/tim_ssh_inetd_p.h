#pragma once

#include "tim_ssh_inetd.h"
#include "tim_a_ssh_inetd_service.h"

#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <libssh/server.h>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>


namespace tim
{

class a_inetd_service;

namespace p
{

struct ssh_inetd;

// Состояние одной живой SSH-сессии: накапливает информацию (pty-параметры,
// user_id из аутентификации) до тех пор, пока клиент не запросит shell —
// тогда создаётся пользовательский сервис.
struct ssh_session_state
{
    explicit ssh_session_state(tim::p::ssh_inetd *owner)
        : _owner(owner)
    {
    }

    tim::p::ssh_inetd *_owner;

    ::ssh_session                                 _session = nullptr;
    ::ssh_channel                                 _channel = nullptr;
    bool                                          _authed = false;
    // Помечается из обработчиков eof/close; реальная очистка происходит вне
    // обработчика libssh в ssh_inetd::dispatch — нельзя освобождать сессию
    // изнутри её же события, libssh ещё работает с её внутренним состоянием.
    bool                                          _pending_close = false;
    tim::uuid                                     _user_id;
    std::string                                   _pub_key;
    std::string                                   _term_name;
    std::size_t                                   _rows = 24;
    std::size_t                                   _cols = 80;
    std::unique_ptr<tim::a_inetd_service>         _service;

    struct ssh_server_callbacks_struct            _server_cb {};
    struct ssh_channel_callbacks_struct           _channel_cb {};
};

struct ssh_inetd
{
    static int on_bind_ready(socket_t fd, int revents, void *userdata);

    static int on_auth_pubkey(::ssh_session session, const char *user, ::ssh_key pubkey,
                              char signature_state, void *userdata);
    static ::ssh_channel on_channel_open(::ssh_session session, void *userdata);

    static int on_pty_request(::ssh_session, ::ssh_channel, const char *term,
                              int x, int y, int px, int py, void *userdata);
    static int on_shell_request(::ssh_session, ::ssh_channel, void *userdata);
    static int on_window_change(::ssh_session, ::ssh_channel, int x, int y, int px, int py, void *userdata);
    static int on_channel_data(::ssh_session, ::ssh_channel, void *data, uint32_t len,
                               int is_stderr, void *userdata);
    static void on_channel_close(::ssh_session, ::ssh_channel, void *userdata);
    static void on_channel_eof(::ssh_session, ::ssh_channel, void *userdata);

    std::string                  _if_addr;
    std::uint16_t                _port = 0;
    std::filesystem::path        _host_key_path;
    ::ssh_bind                   _bind = nullptr;
    ::ssh_event                  _event = nullptr;

    tim::ssh_inetd::service_factory _factory;

    using session_map = std::unordered_map<::ssh_session, std::unique_ptr<ssh_session_state>>;
    session_map _sessions;

    // Счётчик вложенности dispatch(). Освобождать ssh_session/state можно
    // только на самом внешнем уровне — иначе мы рискуем уничтожить ту
    // самую сессию, в чьём же стеке вызовов сейчас находимся (например,
    // если Tcl-скрипт через DISPATCH-обработчик дёрнул нас рекурсивно).
    int _dispatch_depth = 0;
};

}

}
