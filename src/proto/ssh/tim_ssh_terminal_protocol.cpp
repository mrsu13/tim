#include "tim_ssh_terminal_protocol.h"

#include "tim_a_ssh_inetd_service.h"

#include <cassert>
#include <string>


// Открытые

tim::ssh_terminal_protocol::ssh_terminal_protocol(tim::a_ssh_inetd_service *io)
    : tim::a_terminal_protocol(io)
    , _ssh(io)
{
    assert(io);
}

tim::ssh_terminal_protocol::~ssh_terminal_protocol() = default;

const std::string &tim::ssh_terminal_protocol::terminal_name() const
{
    return _ssh->term_name();
}

std::size_t tim::ssh_terminal_protocol::rows() const
{
    return _ssh->rows();
}

std::size_t tim::ssh_terminal_protocol::cols() const
{
    return _ssh->cols();
}

bool tim::ssh_terminal_protocol::write(const char *data, std::size_t size)
{
    assert(data);
    if (!size)
        return true;

    // Преобразуем одиночный LF в CRLF — это поведение, которое раньше
    // прозрачно выполнял libtelnet через telnet_send_text(). В SSH-канал
    // байты идут как есть, поэтому без подмены терминал клиента видит
    // только перевод строки без возврата каретки и текст «лестницей».
    // Если LF уже предваряется CR (вышестоящий слой сам прислал CRLF),
    // ничего не добавляем.
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

void tim::ssh_terminal_protocol::process_raw_data(const char *data, std::size_t size)
{
    // Никакого декодинга — отдаём байты выше как есть.
    if (size)
        data_ready(data, size);
}
