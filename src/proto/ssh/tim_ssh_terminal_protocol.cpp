#include "tim_ssh_terminal_protocol.h"

#include "tim_a_ssh_inetd_service.h"

#include <cassert>
#include <string>


// Открытые

/**
 * Конструктор.
 *
 * \param io SSH-inetd-сервис; источник байт и параметров терминала.
 */
tim::ssh_terminal_protocol::ssh_terminal_protocol(tim::a_ssh_inetd_service *io)
    : tim::a_terminal_protocol(io)
    , _ssh(io)
{
    assert(io);
}

/** Деструктор. */
tim::ssh_terminal_protocol::~ssh_terminal_protocol() = default;

/** \return Имя терминала клиента (значение $TERM). */
const std::string &tim::ssh_terminal_protocol::terminal_name() const
{
    return _ssh->term_name();
}

/** \return Высота окна клиента в строках. */
std::size_t tim::ssh_terminal_protocol::rows() const
{
    return _ssh->rows();
}

/** \return Ширина окна клиента в колонках. */
std::size_t tim::ssh_terminal_protocol::cols() const
{
    return _ssh->cols();
}

/**
 * Пишет данные в SSH-канал (через _ssh->write()).
 *
 * Перед записью одиночный LF превращается в CRLF — это поведение,
 * которое раньше прозрачно выполнял libtelnet через telnet_send_text().
 * В SSH-канал байты идут как есть, поэтому без подмены терминал
 * клиента видит только перевод строки без возврата каретки и текст
 * «лестницей». Если LF уже предваряется CR, ничего не добавляется.
 *
 * \param data Указатель на данные.
 * \param size Размер.
 * \return true при успехе.
 */
bool tim::ssh_terminal_protocol::write(const char *data, std::size_t size)
{
    assert(data);
    if (!size)
        return true;
    std::string out;
    out.reserve(size * 2);
    for (std::size_t i = 0; i < size; ++i)
    {
        const char c = data[i];
        if (c == '\n' && (i == 0 || data[i - 1] != '\r'))
            out.push_back('\r');
        out.push_back(c);
    }
    return _ssh->write(out.data(), out.size());
}

/**
 * Передаёт сырые байты пользователю без обработки — для SSH-канала
 * протокольный layer не нужен. Испускает data_ready напрямую. libssh
 * уже расшифровал их; протокольного слоя поверх не требуется.
 *
 * \param data Сырые байты.
 * \param size Размер.
 */
void tim::ssh_terminal_protocol::process_raw_data(const char *data, std::size_t size)
{
    if (size)
        data_ready(data, size);
}
