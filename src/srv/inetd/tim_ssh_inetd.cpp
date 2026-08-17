#include "tim_ssh_inetd.h"

#include "tim_ssh_inetd_p.h"

#include "tim_a_ssh_inetd_service.h"
#include "tim_config.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_uuid.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <system_error>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>


// Статические помощники

namespace
{

/**
 * Проверяет наличие файла host-ключа; если его нет — генерирует
 * ed25519-ключ и сохраняет на диск с правами owner-read/write.
 *
 * \param path Путь к файлу host-key.
 * \return true при наличии или успешной генерации; false при ошибке.
 */
bool ensure_host_key(const std::filesystem::path &path)
{
    std::error_code ec;
    if (std::filesystem::exists(path, ec))
        return true;

    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec)
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to create directory for SSH host key '%s'."_en,
                         "Не удалось создать каталог для SSH host-ключа '%s'."_ru),
                  path.string().c_str());
        return false;
    }

    ::ssh_key key = nullptr;
    if (ssh_pki_generate(SSH_KEYTYPE_ED25519, 0, &key) != SSH_OK)
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to generate SSH host key for '%s'."_en,
                         "Не удалось сгенерировать SSH host-ключ для '%s'."_ru),
                  path.string().c_str());
        return false;
    }

    // Закрытый ключ не должен быть доступен никому, кроме владельца,
    // ни в один момент времени: сужаем umask до записи файла, иначе
    // между созданием файла и последующим chmod существовало бы окно
    // с правами по умолчанию.
    const mode_t old_umask = ::umask(077);
    const int rc = ssh_pki_export_privkey_file(key, nullptr, nullptr, nullptr, path.string().c_str());
    ::umask(old_umask);
    ssh_key_free(key);

    if (rc != SSH_OK)
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to write SSH host key file '%s'."_en,
                         "Не удалось записать SSH host-ключ '%s'."_ru),
                  path.string().c_str());
        return false;
    }

    std::filesystem::permissions(path,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::replace, ec);
    TIM_TRACE(info, "Generated new SSH host key at '%s'.", path.string().c_str());
    return true;
}

/**
 * Сериализует открытый ключ в OpenSSH-формате без комментария:
 * "<type> <base64>" (например, "ssh-ed25519 AAAA...").
 *
 * \param pubkey Открытый ключ libssh.
 * \return OpenSSH-строка или пустая строка при ошибке.
 */
std::string pubkey_to_openssh(::ssh_key pubkey)
{
    const char *type = ssh_key_type_to_char(ssh_key_type(pubkey));
    if (!type)
        return {};

    char *b64 = nullptr;
    if (ssh_pki_export_pubkey_base64(pubkey, &b64) != SSH_OK || !b64)
        return {};

    std::string out;
    out.reserve(std::strlen(type) + 1 + std::strlen(b64));
    out.append(type);
    out.push_back(' ');
    out.append(b64);
    ssh_string_free_char(b64);
    return out;
}

/**
 * Выводит UUID пользователя из открытого ключа: SHA-256(blob), первые
 * 16 байт, в которые проставлены биты версии 8 и варианта DCE.
 * Версия 8 (RFC 9562) отведена для UUID с нестандартным способом
 * построения — наш идентификатор детерминированно выводится из ключа,
 * поэтому помечать его как случайный (версия 4) было бы некорректно.
 *
 * \param pubkey Открытый ключ libssh.
 * \return UUID; nil-UUID при ошибке хеширования.
 */
tim::uuid uuid_from_pubkey(::ssh_key pubkey)
{
    unsigned char *hash = nullptr;
    std::size_t hlen = 0;
    if (ssh_get_publickey_hash(pubkey, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen) != SSH_OK
            || !hash
            || hlen < 16)
    {
        if (hash)
            ssh_clean_pubkey_hash(&hash);
        return tim::uuid{};
    }

    unsigned char b[16];
    std::memcpy(b, hash, 16);
    ssh_clean_pubkey_hash(&hash);

    // Биты версии (v8 — custom, RFC 9562) и варианта (DCE: 10xxxxxx).
    b[6] = (b[6] & 0x0F) | 0x80;
    b[8] = (b[8] & 0x3F) | 0x80;

    const unsigned int  l = ((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16)
                          | ((unsigned int)b[2] << 8)  |  (unsigned int)b[3];
    const unsigned short w1 = ((unsigned short)b[4] << 8) | b[5];
    const unsigned short w2 = ((unsigned short)b[6] << 8) | b[7];
    return tim::uuid(l, w1, w2,
                     b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
}

}


// Открытые

/** Деструктор. Закрывает все активные сессии и освобождает ssh_bind. */
tim::ssh_inetd::~ssh_inetd()
{
    if (_d->_event)
    {
        for (auto &p: _d->_sessions)
        {
            // Порядок повторяет сборку закрытых сессий в dispatch():
            // сначала уничтожается сервис (его деструктор может писать
            // в канал — прощальное сообщение, восстановление терминала),
            // и лишь затем освобождаются ресурсы libssh.
            p.second->_service.reset();

            ssh_event_remove_session(_d->_event, p.first);
            if (p.second->_channel)
            {
                ssh_channel_free(p.second->_channel);
                p.second->_channel = nullptr;
            }
            ssh_disconnect(p.first);
            ssh_free(p.first);
        }
        _d->_sessions.clear();

        if (_d->_bind)
            ssh_event_remove_fd(_d->_event, ssh_bind_get_fd(_d->_bind));
        ssh_event_free(_d->_event);
        _d->_event = nullptr;
    }
    if (_d->_bind)
    {
        ssh_bind_free(_d->_bind);
        _d->_bind = nullptr;
    }
}

/**
 * Создаёт и запускает inetd. При ошибке открытия host key или
 * прослушивания порта возвращает nullptr (логирует error).
 *
 * \param port TCP-порт для прослушивания.
 * \param host_key_path Путь к ed25519 host-key. Если отсутствует —
 *                     генерируется при первом запуске.
 * \param factory Фабрика сервисов: создаёт прикладной сервис
 *                для каждой аутентифицированной сессии.
 * \param if_addr Интерфейс для bind (пусто = все).
 * \return RAII-указатель на работающий inetd; nullptr при ошибке.
 */
std::unique_ptr<tim::ssh_inetd> tim::ssh_inetd::start(std::uint16_t port,
                                                      const std::filesystem::path &host_key_path,
                                                      service_factory factory,
                                                      const std::string &if_addr)
{
    if (!ensure_host_key(host_key_path))
        return nullptr;

    std::unique_ptr<tim::ssh_inetd> self(
        new tim::ssh_inetd(port, host_key_path, if_addr, std::move(factory)));
    if (!self->_d->_bind || !self->_d->_event)
        return nullptr;
    return self;
}

/**
 * Прерывает работу всех активных сервисов (interrupt() на каждом).
 *
 * Необходимо при завершении сервера, чтобы длительный `while 1 {}` не
 * блокировал выход из exec().
 */
void tim::ssh_inetd::interrupt_all()
{
    for (tim::p::ssh_inetd::session_map::value_type &p: _d->_sessions)
        if (p.second && p.second->_service)
            p.second->_service->interrupt();
}

/**
 * Опрашивает libssh-события (приём новых соединений, рукопожатие,
 * аутентификацию, обмен данными). Должна вызываться периодически
 * из главного цикла приложения.
 *
 * Безопасна к повторному входу: внутренний счётчик dispatch_depth
 * откладывает освобождение сессий до самого внешнего уровня (Tcl-скрипт
 * может рекурсивно вызвать dispatch через DISPATCH-обработчик).
 *
 * Глубина рекурсии ограничена tim::MAX_DISPATCH_DEPTH: иначе ошибочный
 * Tcl-скрипт, бесконечно вызывающий dispatch, переполнил бы стек.
 *
 * \param timeout_ms Максимальное время блокировки в миллисекундах.
 */
void tim::ssh_inetd::dispatch(int timeout_ms)
{
    if (!_d->_event)
        return;

    if (_d->_dispatch_depth >= (int)tim::MAX_DISPATCH_DEPTH)
    {
        TIM_TRACE(warning,
                  TIM_TR("ssh_inetd::dispatch recursion depth %d reached; skipping nested poll."_en,
                         "ssh_inetd::dispatch достиг глубины рекурсии %d; вложенный опрос пропущен."_ru),
                  _d->_dispatch_depth);
        return;
    }

    ++_d->_dispatch_depth;
    ssh_event_dopoll(_d->_event, timeout_ms);
    --_d->_dispatch_depth;

    // Сбор сессий, помеченных к закрытию во время обработчиков. Освобождение
    // ssh_session/ssh_channel изнутри стека событий libssh — UAF: библиотека
    // ещё работает с этими структурами. Кроме того, во время выполнения
    // Tcl-скрипта dispatch() вызывается рекурсивно через DISPATCH-обработчик;
    // в этом случае стек уже может включать сервис закрываемой сессии —
    // sweep отложим до самого внешнего уровня.
    if (_d->_dispatch_depth != 0)
        return;

    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    for (tim::p::ssh_inetd::session_map::iterator it = _d->_sessions.begin();
         it != _d->_sessions.end(); )
    {
        // Сессия, не создавшая прикладной сервис за отведённое время
        // (не завершившая рукопожатие, аутентификацию или запросы
        // pty/shell), закрывается принудительно: TCP keepalive
        // распознаёт только разорванные соединения, а бездействующий
        // клиент удерживал бы дескриптор неограниченно долго.
        if (!it->second->_service
                && !it->second->_pending_close
                && now - it->second->_accepted_at > tim::SSH_SESSION_SETUP_TIMEOUT)
        {
            TIM_TRACE(warning, "%s",
                      TIM_TR("Closing an SSH session that has not completed setup in time."_en,
                             "Закрывается SSH-сессия, не завершившая установление в отведённое время."_ru));
            it->second->_pending_close = true;
        }

        if (!it->second->_pending_close)
        {
            ++it;
            continue;
        }

        // Сначала уничтожаем наш сервис: его деструктор может писать в канал
        // (прощальное сообщение, очистка терминала), поэтому канал должен
        // ещё существовать. Только после этого освобождаем libssh-ресурсы.
        it->second->_service.reset();

        ::ssh_session sess = it->first;
        ssh_event_remove_session(_d->_event, sess);
        if (it->second->_channel)
        {
            ssh_channel_free(it->second->_channel);
            it->second->_channel = nullptr;
        }
        ssh_disconnect(sess);
        ssh_free(sess);

        it = _d->_sessions.erase(it);
    }
}


// Закрытые

/**
 * Закрытый конструктор; экземпляры создаются только через start().
 *
 * \param port TCP-порт.
 * \param host_key_path Путь к host-key.
 * \param if_addr Интерфейс bind.
 * \param factory Фабрика сервисов.
 */
tim::ssh_inetd::ssh_inetd(std::uint16_t port,
                          const std::filesystem::path &host_key_path,
                          const std::string &if_addr,
                          service_factory factory)
    : tim::inetd("ssh_inetd")
    , _d()
{
    assert(port && "port must be non-zero.");
    assert(factory);

    _d->_port = port;
    _d->_if_addr = if_addr.empty() ? "0.0.0.0" : if_addr;
    _d->_host_key_path = host_key_path;
    _d->_factory = std::move(factory);

    _d->_bind = ssh_bind_new();
    if (!_d->_bind)
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to allocate SSH bind."_en,
                         "Не удалось создать SSH bind."_ru));
        return;
    }

    const std::string port_str = std::to_string(_d->_port);
    if (ssh_bind_options_set(_d->_bind, SSH_BIND_OPTIONS_BINDADDR, _d->_if_addr.c_str()) != SSH_OK
            || ssh_bind_options_set(_d->_bind, SSH_BIND_OPTIONS_BINDPORT_STR, port_str.c_str()) != SSH_OK
            || ssh_bind_options_set(_d->_bind, SSH_BIND_OPTIONS_HOSTKEY, _d->_host_key_path.string().c_str()) != SSH_OK)
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to configure SSH bind: %s"_en,
                         "Не удалось настроить SSH bind: %s"_ru),
                  ssh_get_error(_d->_bind));
        ssh_bind_free(_d->_bind);
        _d->_bind = nullptr;
        return;
    }

    if (ssh_bind_listen(_d->_bind) < 0)
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to listen for ssh_inetd at '%s:%u': %s"_en,
                         "Не могу запустить ssh_inetd на '%s:%u': %s"_ru),
                  _d->_if_addr.c_str(), _d->_port, ssh_get_error(_d->_bind));
        ssh_bind_free(_d->_bind);
        _d->_bind = nullptr;
        return;
    }
    ssh_bind_set_blocking(_d->_bind, 0);

    _d->_event = ssh_event_new();
    if (!_d->_event)
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to allocate SSH event poller."_en,
                         "Не удалось создать SSH event poller."_ru));
        ssh_bind_free(_d->_bind);
        _d->_bind = nullptr;
        return;
    }

    if (ssh_event_add_fd(_d->_event, ssh_bind_get_fd(_d->_bind), POLLIN,
                         &tim::p::ssh_inetd::on_bind_ready, _d.get()) != SSH_OK)
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to register SSH bind fd with event poller."_en,
                         "Не удалось добавить SSH bind в event poller."_ru));
        ssh_event_free(_d->_event);
        _d->_event = nullptr;
        ssh_bind_free(_d->_bind);
        _d->_bind = nullptr;
        return;
    }

    TIM_TRACE(debug, "ssh_inetd is listening at '%s:%u'.",
              _d->_if_addr.c_str(), _d->_port);
}

/**
 * libssh-обработчик готовности bind-сокета принять новое соединение.
 * Принимает соединение, выставляет TCP keep-alive, регистрирует
 * callbacks pubkey-аутентификации и channel-open, запускает
 * key-exchange и добавляет сессию в ssh_event.
 */
int tim::p::ssh_inetd::on_bind_ready(socket_t fd, int revents, void *userdata)
{
    (void) fd;
    (void) revents;

    tim::p::ssh_inetd *self = (tim::p::ssh_inetd *)userdata;
    assert(self);

    if (self->_sessions.size() >= tim::SSH_MAX_SESSIONS)
    {
        // Соединение необходимо принять и сразу закрыть: иначе слушающий
        // сокет остался бы в состоянии готовности, а очередь ядра
        // продолжила бы расти.
        ::ssh_session excess = ssh_new();
        if (excess)
        {
            if (ssh_bind_accept(self->_bind, excess) == SSH_OK)
                ssh_disconnect(excess);
            ssh_free(excess);
        }
        TIM_TRACE(warning,
                  TIM_TR("SSH session limit (%zu) reached; a new connection was rejected."_en,
                         "Достигнут предел одновременных SSH-сессий (%zu); новое соединение отклонено."_ru),
                  tim::SSH_MAX_SESSIONS);
        return 0;
    }

    ::ssh_session session = ssh_new();
    if (!session)
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to allocate SSH session."_en,
                         "Не удалось создать SSH-сессию."_ru));
        return 0;
    }

    if (ssh_bind_accept(self->_bind, session) != SSH_OK)
    {
        const char *err = ssh_get_error(self->_bind);
        TIM_TRACE(error,
                  TIM_TR("ssh_bind_accept failed: %s"_en,
                         "ssh_bind_accept не удался: %s"_ru),
                  err ? err : "");
        ssh_free(session);
        return 0;
    }

    // TCP keepalive на принятом сокете: без него исчезнувший клиент (обрыв
    // сети, отсутствие FIN) удерживает SSH-сессию открытой до системного
    // таймаута TCP (на Linux — часы). С keepalive разорванное соединение
    // распознаётся за десятки секунд: ядро отправляет пробу после
    // SSH_KEEPALIVE_IDLE_S секунд бездействия и повторяет SSH_KEEPALIVE_CNT
    // раз с интервалом SSH_KEEPALIVE_INTVL_S; затем сокет закрывается,
    // и libssh штатно срабатывает по close-обработчику.
    const int sock_fd = ssh_get_fd(session);
    if (sock_fd >= 0)
    {
        const int yes = 1;
        setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
        setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPIDLE,
                   &tim::SSH_KEEPALIVE_IDLE_S, sizeof(tim::SSH_KEEPALIVE_IDLE_S));
        setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPINTVL,
                   &tim::SSH_KEEPALIVE_INTVL_S, sizeof(tim::SSH_KEEPALIVE_INTVL_S));
        setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPCNT,
                   &tim::SSH_KEEPALIVE_CNT, sizeof(tim::SSH_KEEPALIVE_CNT));
    }

    auto state = std::make_unique<ssh_session_state>(self);
    state->_session = session;

    state->_server_cb.userdata = state.get();
    state->_server_cb.auth_pubkey_function = &tim::p::ssh_inetd::on_auth_pubkey;
    state->_server_cb.channel_open_request_session_function = &tim::p::ssh_inetd::on_channel_open;
    ssh_callbacks_init(&state->_server_cb);
    ssh_set_server_callbacks(session, &state->_server_cb);

    ssh_set_auth_methods(session, SSH_AUTH_METHOD_PUBLICKEY);
    ssh_set_blocking(session, 0);

    if (ssh_handle_key_exchange(session) == SSH_ERROR)
    {
        // SSH_AGAIN продолжит в event loop; SSH_ERROR — обрыв.
        const int err = ssh_get_error_code(session);
        if (err != SSH_AGAIN)
        {
            TIM_TRACE(error,
                      TIM_TR("SSH key exchange failed: %s"_en,
                             "SSH key exchange не удался: %s"_ru),
                      ssh_get_error(session));
            ssh_disconnect(session);
            ssh_free(session);
            return 0;
        }
    }

    if (ssh_event_add_session(self->_event, session) != SSH_OK)
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to add SSH session to event poller."_en,
                         "Не удалось добавить SSH-сессию в event poller."_ru));
        ssh_disconnect(session);
        ssh_free(session);
        return 0;
    }

    TIM_TRACE(debug, "ssh_inetd accepted a connection.");
    self->_sessions.emplace(session, std::move(state));
    return 0;
}

/**
 * libssh-обработчик pubkey-аутентификации. Любой валидный ключ
 * принимается: идентификатор пользователя выводится из SHA-256
 * самого ключа (uuid_from_pubkey).
 */
int tim::p::ssh_inetd::on_auth_pubkey(::ssh_session session, const char *user, ::ssh_key pubkey,
                                      char signature_state, void *userdata)
{
    (void) user;

    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);
    assert(st->_session == session);

    if (signature_state == SSH_PUBLICKEY_STATE_NONE)
    {
        // Клиент только проверяет, готовы ли мы принять этот ключ.
        return SSH_AUTH_SUCCESS;
    }
    if (signature_state != SSH_PUBLICKEY_STATE_VALID)
        return SSH_AUTH_DENIED;

    const tim::uuid id = uuid_from_pubkey(pubkey);
    if (!id.valid())
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to derive user id from SSH pubkey."_en,
                         "Не удалось вычислить идентификатор пользователя из SSH-ключа."_ru));
        return SSH_AUTH_DENIED;
    }

    st->_user_id = id;
    st->_pub_key = pubkey_to_openssh(pubkey);
    st->_authed = true;
    TIM_TRACE(debug, "SSH user authenticated: %s.", id.to_string().c_str());
    return SSH_AUTH_SUCCESS;
}

/**
 * libssh-обработчик открытия канала. Только аутентифицированному
 * клиенту и только одному каналу на сессию. Регистрирует callbacks
 * pty/shell/window/data/close/eof.
 */
::ssh_channel tim::p::ssh_inetd::on_channel_open(::ssh_session session, void *userdata)
{
    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);
    assert(st->_session == session);

    if (!st->_authed || st->_channel)
        return nullptr;

    ::ssh_channel channel = ssh_channel_new(session);
    if (!channel)
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to allocate SSH channel."_en,
                         "Не удалось создать SSH-канал."_ru));
        return nullptr;
    }

    st->_channel = channel;

    st->_channel_cb.userdata = st;
    st->_channel_cb.channel_pty_request_function = &tim::p::ssh_inetd::on_pty_request;
    st->_channel_cb.channel_shell_request_function = &tim::p::ssh_inetd::on_shell_request;
    st->_channel_cb.channel_pty_window_change_function = &tim::p::ssh_inetd::on_window_change;
    st->_channel_cb.channel_data_function = &tim::p::ssh_inetd::on_channel_data;
    st->_channel_cb.channel_close_function = &tim::p::ssh_inetd::on_channel_close;
    st->_channel_cb.channel_eof_function = &tim::p::ssh_inetd::on_channel_eof;
    ssh_callbacks_init(&st->_channel_cb);
    ssh_set_channel_callbacks(channel, &st->_channel_cb);

    return channel;
}

int tim::p::ssh_inetd::on_pty_request(::ssh_session, ::ssh_channel, const char *term,
                                      int x, int y, int /*px*/, int /*py*/, void *userdata)
{
    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);

    st->_term_name = term ? term : "";
    st->_cols = (std::size_t)x;
    st->_rows = (std::size_t)y;

    TIM_TRACE(debug, "SSH pty-req: term='%s', %dx%d.", term ? term : "", x, y);
    return SSH_OK;
}

int tim::p::ssh_inetd::on_shell_request(::ssh_session, ::ssh_channel channel, void *userdata)
{
    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);

    if (st->_service)
        return SSH_ERROR; // Уже создан.

    tim::ssh_session_info info{
        .session = st->_session,
        .channel = channel,
        .user_id = st->_user_id,
        .pub_key = st->_pub_key,
        .term_name = st->_term_name,
        .rows = st->_rows,
        .cols = st->_cols,
    };

    st->_service = st->_owner->_factory(info);
    if (!st->_service)
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to instantiate SSH service."_en,
                         "Не удалось создать SSH-сервис."_ru));
        return SSH_ERROR;
    }

    TIM_TRACE(debug, "SSH shell ready for user %s.", st->_user_id.to_string().c_str());
    return SSH_OK;
}

/**
 * libssh-обработчик window-change. Обновляет геометрию терминала
 * клиента в ssh_session_state и пробрасывает в сервис (если он уже создан).
 */
int tim::p::ssh_inetd::on_window_change(::ssh_session, ::ssh_channel, int x, int y,
                                        int /*px*/, int /*py*/, void *userdata)
{
    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);

    st->_cols = (std::size_t)x;
    st->_rows = (std::size_t)y;

    if (st->_service)
        ((tim::a_ssh_inetd_service *)st->_service.get())->on_window_change((std::size_t)y, (std::size_t)x);

    return SSH_OK;
}

/**
 * libssh-обработчик прихода байт в канал. Передаёт данные в сервис
 * через on_channel_data() (a_ssh_inetd_service). stderr-байты игнорирует.
 */
int tim::p::ssh_inetd::on_channel_data(::ssh_session, ::ssh_channel, void *data, uint32_t len,
                                       int is_stderr, void *userdata)
{
    if (is_stderr)
        return (int)len;

    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);

    if (!st->_service)
        return 0; // Сервис ещё не создан — пусть libssh сохранит данные у себя.

    ((tim::a_ssh_inetd_service *)st->_service.get())->on_channel_data((const char *)data, (std::size_t)len);
    return (int)len;
}

/**
 * libssh-обработчик закрытия канала. Прерывает сервис (если он есть)
 * и помечает сессию _pending_close; реальная очистка произойдёт
 * в dispatch() на внешнем уровне вложенности.
 */
void tim::p::ssh_inetd::on_channel_close(::ssh_session, ::ssh_channel, void *userdata)
{
    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);

    TIM_TRACE(debug, "SSH channel closed.");
    if (st->_service)
        st->_service->interrupt();
    st->_pending_close = true;
}

/**
 * libssh-обработчик EOF на канале. Поведение идентично on_channel_close —
 * libssh может позвать одно, другое или оба, в зависимости от поведения
 * клиента.
 */
void tim::p::ssh_inetd::on_channel_eof(::ssh_session, ::ssh_channel, void *userdata)
{
    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);

    TIM_TRACE(debug, "SSH channel EOF.");
    if (st->_service)
        st->_service->interrupt();
    st->_pending_close = true;
}
