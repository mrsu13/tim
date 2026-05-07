#include "tim_ssh_terminal_protocol.h"

#include "tim_a_ssh_inetd_service.h"

#include <cassert>


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
    return _ssh->write(data, size);
}

void tim::ssh_terminal_protocol::process_raw_data(const char *data, std::size_t size)
{
    // Никакого декодинга — отдаём байты выше как есть.
    if (size)
        data_ready(data, size);
}
