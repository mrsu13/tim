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

    /**
     * Закрывает SSH-канал и сессию. Идемпотентно.
     */
    void close() override;

    /**
     * Возвращает срез накопленных в _recv_buf байтов.
     *
     * \param data Сюда записывается указатель на буфер.
     * \return Количество байт в буфере; после чтения буфер очищается.
     */
    std::size_t read(const char **data) override;

    /**
     * Пишет байты в SSH-канал.
     *
     * \param data Указатель на данные.
     * \param size Размер данных в байтах.
     * \return true при успехе.
     */
    bool write(const char *data, std::size_t size) override;

    /** \return UUID пользователя сессии. */
    const tim::uuid &user_id() const noexcept;
    /** \return Открытый ключ клиента (OpenSSH-формат). */
    const std::string &pub_key() const noexcept;
    /** \return $TERM клиента. */
    const std::string &term_name() const noexcept;
    /** \return Высота терминала клиента в строках. */
    std::size_t rows() const noexcept;
    /** \return Ширина терминала клиента в колонках. */
    std::size_t cols() const noexcept;

    /**
     * Вызывается ssh_inetd при поступлении байтов в канал. Кладёт
     * их во внутренний буфер и испускает сигнал data_ready (через
     * a_io_device).
     *
     * \param data Указатель на принятые байты.
     * \param size Размер принятых данных.
     */
    void on_channel_data(const char *data, std::size_t size);

    /**
     * Вызывается ssh_inetd при изменении размеров терминала клиента.
     *
     * \param rows Новая высота в строках.
     * \param cols Новая ширина в колонках.
     */
    void on_window_change(std::size_t rows, std::size_t cols) noexcept;

protected:

    /**
     * Конструктор только для наследников.
     *
     * \param name Имя сервиса.
     * \param info Параметры открытой SSH-сессии.
     */
    a_ssh_inetd_service(const std::string &name, const tim::ssh_session_info &info);

    /** Деструктор для наследников. */
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
