#include "tim_ssh_inetd.h"

#include "tim_ssh_inetd_p.h"

#include "tim_a_ssh_inetd_service.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_uuid.h"

#include <cassert>
#include <cstring>
#include <system_error>

#include <poll.h>


// Статические помощники

namespace
{

// Проверяет наличие файла host-ключа; если его нет — генерирует ed25519
// ключ и сохраняет на диск. Возвращает true при успехе.
bool ensure_host_key(const std::filesystem::path &path)
{
    std::error_code ec;
    if (std::filesystem::exists(path, ec))
        return true;

    std::filesystem::create_directories(path.parent_path(), ec);

    ::ssh_key key = nullptr;
    if (ssh_pki_generate(SSH_KEYTYPE_ED25519, 0, &key) != SSH_OK)
    {
        TIM_TRACE(Error,
                  TIM_TR("Failed to generate SSH host key for '%s'."_en,
                         "Не удалось сгенерировать SSH host-ключ для '%s'."_ru),
                  path.string().c_str());
        return false;
    }

    const int rc = ssh_pki_export_privkey_file(key, nullptr, nullptr, nullptr, path.string().c_str());
    ssh_key_free(key);

    if (rc != SSH_OK)
    {
        TIM_TRACE(Error,
                  TIM_TR("Failed to write SSH host key file '%s'."_en,
                         "Не удалось записать SSH host-ключ '%s'."_ru),
                  path.string().c_str());
        return false;
    }

    std::filesystem::permissions(path,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                 std::filesystem::perm_options::replace, ec);
    TIM_TRACE(Info, "Generated new SSH host key at '%s'.", path.string().c_str());
    return true;
}

// Сериализует открытый ключ в OpenSSH-формате без комментария:
// "<type> <base64>" (например, "ssh-ed25519 AAAA...").
// Возвращает пустую строку при ошибке.
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

// Выводит UUID пользователя из открытого ключа: SHA-256(blob), первые 16
// байт, в которые проставлены биты версии 4 и варианта DCE.
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

    // Биты версии (v4) и варианта (DCE: 10xxxxxx).
    b[6] = (b[6] & 0x0F) | 0x40;
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

tim::ssh_inetd::~ssh_inetd()
{
    if (_d->_event)
    {
        for (auto &p: _d->_sessions)
        {
            ssh_event_remove_session(_d->_event, p.first);
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

void tim::ssh_inetd::dispatch(int timeout_ms)
{
    if (!_d->_event)
        return;

    ssh_event_dopoll(_d->_event, timeout_ms);

    // Сбор сессий, помеченных к закрытию во время колбэков. Освобождение
    // ssh_session/ssh_channel изнутри стека событий libssh — UAF: библиотека
    // ещё работает с этими структурами. Делаем это здесь, после dopoll.
    for (tim::p::ssh_inetd::session_map::iterator it = _d->_sessions.begin();
         it != _d->_sessions.end(); )
    {
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

tim::ssh_inetd::ssh_inetd(std::uint16_t port,
                          const std::filesystem::path &host_key_path,
                          const std::string &if_addr,
                          service_factory factory)
    : tim::inetd("ssh_inetd")
    , _d(new tim::p::ssh_inetd())
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
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to allocate SSH bind."_en,
                         "Не удалось создать SSH bind."_ru));
        return;
    }

    const std::string port_str = std::to_string(_d->_port);
    if (ssh_bind_options_set(_d->_bind, SSH_BIND_OPTIONS_BINDADDR, _d->_if_addr.c_str()) != SSH_OK
            || ssh_bind_options_set(_d->_bind, SSH_BIND_OPTIONS_BINDPORT_STR, port_str.c_str()) != SSH_OK
            || ssh_bind_options_set(_d->_bind, SSH_BIND_OPTIONS_HOSTKEY, _d->_host_key_path.string().c_str()) != SSH_OK)
    {
        TIM_TRACE(Error,
                  TIM_TR("Failed to configure SSH bind: %s"_en,
                         "Не удалось настроить SSH bind: %s"_ru),
                  ssh_get_error(_d->_bind));
        ssh_bind_free(_d->_bind);
        _d->_bind = nullptr;
        return;
    }

    if (ssh_bind_listen(_d->_bind) < 0)
    {
        TIM_TRACE(Error,
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
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to allocate SSH event poller."_en,
                         "Не удалось создать SSH event poller."_ru));
        ssh_bind_free(_d->_bind);
        _d->_bind = nullptr;
        return;
    }

    if (ssh_event_add_fd(_d->_event, ssh_bind_get_fd(_d->_bind), POLLIN,
                         &tim::p::ssh_inetd::on_bind_ready, _d.get()) != SSH_OK)
    {
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to register SSH bind fd with event poller."_en,
                         "Не удалось добавить SSH bind в event poller."_ru));
        ssh_event_free(_d->_event);
        _d->_event = nullptr;
        ssh_bind_free(_d->_bind);
        _d->_bind = nullptr;
        return;
    }

    TIM_TRACE(Debug, "ssh_inetd is listening at '%s:%u'.",
              _d->_if_addr.c_str(), _d->_port);
}

int tim::p::ssh_inetd::on_bind_ready(socket_t fd, int revents, void *userdata)
{
    (void) fd;
    (void) revents;

    tim::p::ssh_inetd *self = (tim::p::ssh_inetd *)userdata;
    assert(self);

    ::ssh_session session = ssh_new();
    if (!session)
    {
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to allocate SSH session."_en,
                         "Не удалось создать SSH-сессию."_ru));
        return 0;
    }

    if (ssh_bind_accept(self->_bind, session) != SSH_OK)
    {
        const char *err = ssh_get_error(self->_bind);
        TIM_TRACE(Error,
                  TIM_TR("ssh_bind_accept failed: %s"_en,
                         "ssh_bind_accept не удался: %s"_ru),
                  err ? err : "");
        ssh_free(session);
        return 0;
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
            TIM_TRACE(Error,
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
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to add SSH session to event poller."_en,
                         "Не удалось добавить SSH-сессию в event poller."_ru));
        ssh_disconnect(session);
        ssh_free(session);
        return 0;
    }

    TIM_TRACE(Debug, "ssh_inetd accepted a connection.");
    self->_sessions.emplace(session, std::move(state));
    return 0;
}

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
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to derive user id from SSH pubkey."_en,
                         "Не удалось вычислить идентификатор пользователя из SSH-ключа."_ru));
        return SSH_AUTH_DENIED;
    }

    st->_user_id = id;
    st->_pub_key = pubkey_to_openssh(pubkey);
    st->_authed = true;
    TIM_TRACE(Debug, "SSH user authenticated: %s.", id.to_string().c_str());
    return SSH_AUTH_SUCCESS;
}

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
        TIM_TRACE(Error, "%s",
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

    TIM_TRACE(Debug, "SSH pty-req: term='%s', %dx%d.", term ? term : "", x, y);
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
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to instantiate SSH service."_en,
                         "Не удалось создать SSH-сервис."_ru));
        return SSH_ERROR;
    }

    TIM_TRACE(Debug, "SSH shell ready for user %s.", st->_user_id.to_string().c_str());
    return SSH_OK;
}

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

void tim::p::ssh_inetd::on_channel_close(::ssh_session, ::ssh_channel, void *userdata)
{
    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);

    TIM_TRACE(Debug, "SSH channel closed.");
    st->_pending_close = true;
}

void tim::p::ssh_inetd::on_channel_eof(::ssh_session, ::ssh_channel, void *userdata)
{
    ssh_session_state *st = (ssh_session_state *)userdata;
    assert(st);

    TIM_TRACE(Debug, "SSH channel EOF.");
    st->_pending_close = true;
}
