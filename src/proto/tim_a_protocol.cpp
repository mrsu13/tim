#include "tim_a_protocol.h"

#include "tim_a_protocol_p.h"

#include "tim_a_io_device.h"

#include <cassert>


// Public

tim::a_protocol::a_protocol(tim::a_io_device *io)
    : data_ready()
    , _d(new tim::p::a_protocol(this))
{
    assert(io);

    _d->_io = io;
    _d->_io->ready_read.connect(std::bind(&tim::p::a_protocol::on_ready_read, _d.get()));
}

tim::a_protocol::~a_protocol() = default;

tim::a_io_device *tim::a_protocol::io() const
{
    return _d->_io;
}

bool tim::a_protocol::write_str(const std::string &s)
{
    return s.empty()
                ? true
                : write(s.c_str(), s.size());
}


// Private

void tim::p::a_protocol::on_ready_read()
{
    const char *data;
    const std::size_t size = _io->read(&data);
    _q->process_raw_data(data, size);
}
