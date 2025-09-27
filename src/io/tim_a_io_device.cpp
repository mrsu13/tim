#include "tim_a_io_device.h"

#include "tim_a_io_device_p.h"

#include "tim_trace.h"

#include "mongoose.h"

#include <cassert>


// Public

tim::a_io_device::~a_io_device()
{
    if (_d->_c)
        _d->_c->is_draining = 1;
}

mg_connection *tim::a_io_device::connection() const
{
    return _d->_c;
}

void tim::a_io_device::close()
{
    _d->_c->is_draining = 1;
}

bool tim::a_io_device::read_from_connection()
{
    struct mg_iobuf *r = &(_d->_c->recv);
    // TIM_TRACE(Debug, "'%s' service got data: '%.*s'", name().c_str(), (int)r->len, r->buf);
    std::size_t bytes_read = 0;
    if (!ready_read((const char *)r->buf, r->len, &bytes_read))
        return false;

    r->len -= bytes_read; // Tell Mongoose we've consumed data.
    return true;
}

bool tim::a_io_device::write(const char *data, std::size_t size, std::size_t *bytes_written)
{
    assert(data);
    return ready_write(data, size, bytes_written);
}

bool tim::a_io_device::write_str(const std::string &s)
{
    return s.empty()
                ? true
                : write(s.c_str(), s.size());
}


// Protected

tim::a_io_device::a_io_device(mg_connection *c)
    : _d(new tim::p::a_io_device())
{
    assert(c);
    _d->_c = c;
}
