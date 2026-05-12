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

// Параметры SSH-сессии, передаваемые в фабрику сервиса в момент,
// когда клиент уже аутентифицирован и запросил shell.
// pub_key — открытый ключ клиента в OpenSSH-формате ("<type> <base64>"),
// без комментария.
struct ssh_session_info
{
    ssh_session  session;
    ssh_channel  channel;
    tim::uuid    user_id;
    std::string  pub_key;
    std::string  term_name;
    std::size_t  rows;
    std::size_t  cols;
};

// Реализация a_inetd_service поверх SSH-канала libssh. read() отдаёт байты,
// которые libssh принёс через обработчик channel_data; write() пишет в канал.
// Идентификатор пользователя (user_id) — UUID, выведенный из SHA-256
// открытого ключа клиента.
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

    // Вызывается ssh_inetd при поступлении байтов в канал. Кладёт их во
    // внутренний буфер и эмитит ready_read.
    void on_channel_data(const char *data, std::size_t size);

    // Вызывается ssh_inetd при изменении размеров терминала клиента.
    void on_window_change(std::size_t rows, std::size_t cols) noexcept;

protected:

    a_ssh_inetd_service(const std::string &name, const tim::ssh_session_info &info);
    ~a_ssh_inetd_service();

private:

    ssh_session         _session;
    ssh_channel         _channel;
    tim::uuid           _user_id;
    std::string         _pub_key;
    std::string         _term_name;
    std::size_t         _rows;
    std::size_t         _cols;
    std::vector<char>   _recv_buf;
};

}
