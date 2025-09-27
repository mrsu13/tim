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

std::size_t tim::a_io_device::read(const char **data)
{
    assert(data);

    struct mg_iobuf *r = &(_d->_c->recv);
    *data = (const char *)r->buf;
    const std::size_t size = r->len;
    r->len = 0; // Tell Mongoose we've consumed data.
    return size;
}

bool tim::a_io_device::write(const char *data, std::size_t size)
{
    assert(data);

    return size
                ? mg_send(_d->_c, data, size)
                : true;
}

bool tim::a_io_device::write_str(const std::string &s)
{
    return s.empty()
                ? true
                : write(s.c_str(), s.size());
}


// Protected

tim::a_io_device::a_io_device(mg_connection *c)
    : ready_read()
    , _d(new tim::p::a_io_device())
{
    assert(c);

    _d->_c = c;
}
