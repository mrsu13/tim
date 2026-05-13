#pragma once

#include "tim_a_inetd_service.h"
#include "tim_uuid.h"

#include <cstddef>
#include <string>
#include <vector>


typedef struct ssh_session_struct *ssh_session;
typedef struct ssh_channel_struct *ssh_channel;


namespace tim
{

/**
 * Параметры SSH-сессии, передаваемые в фабрику сервиса.
 *
 * Заполняется ssh_inetd в момент, когда клиент уже аутентифицирован
 * и запросил shell. \a pub_key — открытый ключ клиента в OpenSSH-формате
 * ("<type> <base64>"), без комментария.
 */
struct ssh_session_info
{
    /** Активная libssh-сессия с клиентом. */
    ssh_session  session;
    /** Запрошенный клиентом shell-канал. */
    ssh_channel  channel;
    /** UUID пользователя, выведенный из SHA-256(pub_key). */
    tim::uuid    user_id;
    /** Открытый ключ клиента (OpenSSH-формат, без комментария). */
    std::string  pub_key;
    /** $TERM, переданный клиентом (например, "xterm-256color"). */
    std::string  term_name;
    /** Высота терминала клиента в строках. */
    std::size_t  rows;
    /** Ширина терминала клиента в колонках. */
    std::size_t  cols;
};

/**
 * Реализация a_inetd_service поверх SSH-канала libssh.
 *
 * read() отдаёт байты, которые libssh принёс через обработчик
 * channel_data; write() пишет в канал. Идентификатор пользователя
 * (user_id) — UUID, выведенный из SHA-256 открытого ключа клиента.
 */
class a_ssh_inetd_service : public tim::a_inetd_service
{

public:

    void close() override;

    std::size_t read(const char **data) override;

    bool write(const char *data, std::size_t size) override;

    const tim::uuid &user_id() const noexcept;
    const std::string &pub_key() const noexcept;
    const std::string &term_name() const noexcept;
    std::size_t rows() const noexcept;
    std::size_t cols() const noexcept;

    void on_channel_data(const char *data, std::size_t size);

    void on_window_change(std::size_t rows, std::size_t cols) noexcept;

protected:

    a_ssh_inetd_service(const std::string &name, const tim::ssh_session_info &info);

    ~a_ssh_inetd_service();

private:

    /** libssh-сессия. */
    ssh_session         _session;
    /** Активный shell-канал. */
    ssh_channel         _channel;
    /** UUID пользователя. */
    tim::uuid           _user_id;
    /** Открытый ключ клиента. */
    std::string         _pub_key;
    /** $TERM клиента. */
    std::string         _term_name;
    /** Высота терминала клиента. */
    std::size_t         _rows;
    /** Ширина терминала клиента. */
    std::size_t         _cols;
    /** Буфер накопленных входящих байтов до их чтения через read(). */
    std::vector<char>   _recv_buf;
};

}
