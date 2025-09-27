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

void tim::a_io_device::read()
{
    struct mg_iobuf *r = &(_d->_c->recv);
    // TIM_TRACE(Debug, "'%s' service got data: '%.*s'", name().c_str(), (int)r->len, r->buf);

    r->len -= _d->_data_handler((const char *)r->buf, r->len); // Tell Mongoose we've consumed data.
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

tim::a_io_device::a_io_device(mg_connection *c, data_handler dh)
    : _d(new tim::p::a_io_device())
{
    assert(c);
    assert(dh);

    _d->_c = c;
    _d->_data_handler = dh;
}
