#include "tim_a_ssh_inetd_service.h"

#include <libssh/libssh.h>

#include <cassert>


// Открытые

void tim::a_ssh_inetd_service::close()
{
    if (_channel)
    {
        if (ssh_channel_is_open(_channel) && !ssh_channel_is_closed(_channel))
            ssh_channel_send_eof(_channel);
        ssh_channel_close(_channel);
    }
}

std::size_t tim::a_ssh_inetd_service::read(const char **data)
{
    assert(data);

    *data = _recv_buf.data();
    const std::size_t size = _recv_buf.size();
    _recv_buf.clear();
    return size;
}

bool tim::a_ssh_inetd_service::write(const char *data, std::size_t size)
{
    assert(data);

    if (!size || !_channel)
        return true;

    const int rc = ssh_channel_write(_channel, data, (uint32_t)size);
    return rc == (int)size;
}

const tim::uuid &tim::a_ssh_inetd_service::user_id() const noexcept
{
    return _user_id;
}

const std::string &tim::a_ssh_inetd_service::term_name() const noexcept
{
    return _term_name;
}

std::size_t tim::a_ssh_inetd_service::rows() const noexcept
{
    return _rows;
}

std::size_t tim::a_ssh_inetd_service::cols() const noexcept
{
    return _cols;
}

void tim::a_ssh_inetd_service::on_channel_data(const char *data, std::size_t size)
{
    if (!size)
        return;

    _recv_buf.insert(_recv_buf.end(), data, data + size);
    ready_read();
}

void tim::a_ssh_inetd_service::on_window_change(std::size_t rows, std::size_t cols) noexcept
{
    _rows = rows;
    _cols = cols;
}


// Защищённые

tim::a_ssh_inetd_service::a_ssh_inetd_service(const std::string &name, const tim::ssh_session_info &info)
    : tim::a_inetd_service(name)
    , _session(info.session)
    , _channel(info.channel)
    , _user_id(info.user_id)
    , _term_name(info.term_name)
    , _rows(info.rows)
    , _cols(info.cols)
{
    assert(_session);
    assert(_channel);
}

tim::a_ssh_inetd_service::~a_ssh_inetd_service() = default;
