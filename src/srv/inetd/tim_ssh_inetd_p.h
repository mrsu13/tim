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

/**
 * Состояние одной живой SSH-сессии.
 *
 * Накапливает информацию (pty-параметры, user_id из аутентификации)
 * до тех пор, пока клиент не запросит shell — тогда создаётся
 * пользовательский сервис.
 */
struct ssh_session_state
{
    /**
     * \param owner Указатель на родительский ssh_inetd (для доступа
     *              к фабрике и общей карте сессий).
     */
    explicit ssh_session_state(tim::p::ssh_inetd *owner)
        : _owner(owner)
    {
    }

    /** Родительский ssh_inetd. */
    tim::p::ssh_inetd *_owner;

    /** libssh-сессия. */
    ::ssh_session                                 _session = nullptr;
    /** SSH-канал; nullptr пока shell не открыт. */
    ::ssh_channel                                 _channel = nullptr;
    /** true после успешной pubkey-аутентификации. */
    bool                                          _authed = false;
    /**
     * Помечается из обработчиков eof/close; реальная очистка происходит
     * вне обработчика libssh в ssh_inetd::dispatch — нельзя освобождать
     * сессию изнутри её же события, libssh ещё работает с её внутренним
     * состоянием.
     */
    bool                                          _pending_close = false;
    /** UUID пользователя (SHA-256 от открытого ключа). */
    tim::uuid                                     _user_id;
    /** Открытый ключ клиента в OpenSSH-форме. */
    std::string                                   _pub_key;
    /** $TERM, переданный клиентом через pty-req. */
    std::string                                   _term_name;
    /** Высота окна (rows), значение по умолчанию 24. */
    std::size_t                                   _rows = 24;
    /** Ширина окна (cols), значение по умолчанию 80. */
    std::size_t                                   _cols = 80;
    /** Пользовательский сервис, обслуживающий канал (создан в shell-req). */
    std::unique_ptr<tim::a_inetd_service>         _service;

    /** Структура обработчиков для libssh server callbacks. */
    struct ssh_server_callbacks_struct            _server_cb {};
    /** Структура обработчиков для libssh channel callbacks. */
    struct ssh_channel_callbacks_struct           _channel_cb {};
};

/**
 * Внутреннее состояние tim::ssh_inetd (PIMPL).
 *
 * Содержит ssh_bind, ssh_event, фабрику сервисов и карту живых сессий.
 * Все статические методы — это callbacks libssh, принимающие userdata,
 * через который восстанавливается ssh_inetd-владелец.
 */
struct ssh_inetd
{
    /**
     * Callback: bind-сокет готов принять новое соединение.
     *
     * \param fd Сырой fd сокета.
     * \param revents Маска событий poll.
     * \param userdata Указатель на p::ssh_inetd.
     * \return 0 для libssh.
     */
    static int on_bind_ready(socket_t fd, int revents, void *userdata);

    /**
     * Callback: pubkey-аутентификация. Любой ключ принимается — UUID
     * пользователя выводится из SHA-256(pub_key).
     *
     * \param session libssh-сессия.
     * \param user Имя SSH-пользователя (игнорируется).
     * \param pubkey Открытый ключ.
     * \param signature_state Стадия (probe / final).
     * \param userdata Указатель на ssh_session_state.
     * \return SSH_AUTH_SUCCESS / SSH_AUTH_DENIED.
     */
    static int on_auth_pubkey(::ssh_session session, const char *user, ::ssh_key pubkey,
                              char signature_state, void *userdata);

    /**
     * Callback: клиент запрашивает открытие канала.
     *
     * \param session libssh-сессия.
     * \param userdata Указатель на ssh_session_state.
     * \return Открытый канал или nullptr.
     */
    static ::ssh_channel on_channel_open(::ssh_session session, void *userdata);

    /**
     * Callback: pty-request. Сохраняет $TERM и геометрию.
     *
     * \param term $TERM от клиента.
     * \param x Ширина в колонках.
     * \param y Высота в строках.
     * \param px Ширина в пикселях (не используем).
     * \param py Высота в пикселях (не используем).
     * \param userdata Указатель на ssh_session_state.
     * \return SSH_OK.
     */
    static int on_pty_request(::ssh_session, ::ssh_channel, const char *term,
                              int x, int y, int px, int py, void *userdata);

    /**
     * Callback: shell-request. Создаёт пользовательский сервис через
     * фабрику и сохраняет его в ssh_session_state.
     *
     * \param userdata Указатель на ssh_session_state.
     * \return SSH_OK при успехе.
     */
    static int on_shell_request(::ssh_session, ::ssh_channel, void *userdata);

    /**
     * Callback: window-change. Обновляет геометрию терминала клиента.
     *
     * \param x Новая ширина в колонках.
     * \param y Новая высота в строках.
     * \param px Ширина в пикселях.
     * \param py Высота в пикселях.
     * \param userdata Указатель на ssh_session_state.
     * \return SSH_OK.
     */
    static int on_window_change(::ssh_session, ::ssh_channel, int x, int y, int px, int py, void *userdata);

    /**
     * Callback: принятые из канала байты. Передаёт сервису через
     * on_channel_data().
     *
     * \param data Указатель на байты.
     * \param len Размер.
     * \param is_stderr stderr-флаг (мы не различаем).
     * \param userdata Указатель на ssh_session_state.
     * \return Число обработанных байт.
     */
    static int on_channel_data(::ssh_session, ::ssh_channel, void *data, uint32_t len,
                               int is_stderr, void *userdata);

    /**
     * Callback: канал закрыт. Помечает _pending_close — реальная
     * очистка позже в dispatch().
     *
     * \param userdata Указатель на ssh_session_state.
     */
    static void on_channel_close(::ssh_session, ::ssh_channel, void *userdata);

    /**
     * Callback: EOF на канале. Помечает _pending_close.
     *
     * \param userdata Указатель на ssh_session_state.
     */
    static void on_channel_eof(::ssh_session, ::ssh_channel, void *userdata);

    /** IP-адрес/имя интерфейса для bind (пусто = все). */
    std::string                  _if_addr;
    /** TCP-порт. */
    std::uint16_t                _port = 0;
    /** Путь к host-key. */
    std::filesystem::path        _host_key_path;
    /** libssh ssh_bind-объект. */
    ::ssh_bind                   _bind = nullptr;
    /** libssh ssh_event для опроса соединений. */
    ::ssh_event                  _event = nullptr;

    /** Фабрика прикладных сервисов. */
    tim::ssh_inetd::service_factory _factory;

    /** Карта живых сессий: ssh_session* → state. */
    using session_map = std::unordered_map<::ssh_session, std::unique_ptr<ssh_session_state>>;
    /** Хранилище живых сессий. */
    session_map _sessions;

    /**
     * Счётчик вложенности dispatch(). Освобождать ssh_session/state
     * можно только на самом внешнем уровне — иначе мы рискуем уничтожить
     * ту самую сессию, в чьём же стеке вызовов сейчас находимся
     * (например, если Tcl-скрипт через DISPATCH-обработчик дёрнул
     * нас рекурсивно).
     */
    int _dispatch_depth = 0;
};

}

}
