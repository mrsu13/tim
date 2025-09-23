#include "tim_a_inetd_service.h"

#include "tim_a_inetd_service_p.h"

#include "tim_trace.h"

#include "mongoose.h"

#include <cassert>


// Public

tim::a_inetd_service::~a_inetd_service()
{
    if (_d->_c)
        _d->_d->is_draining = 1;
}

mg_connection *tim::a_inetd_service::connection() const
{
    return _d->_c;
}

void tim::a_inetd_service::close()
{
    _d->_c->is_draining = 1;
}

bool tim::a_inetd_service::read()
{
    struct mg_iobuf *r = &(_d->_c->recv);
    TIM_TRACE(Debug, "%s service got data: '%.*s'", name().c_str(), (int)r->len, r->buf);
    std::size_t bytes_read = 0;
    if (!ready_read(srv, (const char *)r->buf, r->len, &bytes_read))
        return false;

    r->len -= bytes_read; // Tell Mongoose we've consumed data.
    return true;
}

bool tim::a_inetd_service::write(const char *data, std::size_t size, std::size_t *bytes_written)
{
    assert(data);

    return ready_write(srv, data, size, bytes_written);
}

bool tim::a_inetd_service::write_str(const std::string &s)
{
    return s.empty()
                ? true
                : write(s.c_str(), s.size());
}


// Protected

tim::a_inetd_service::a_inetd_service(const std::string &name, mg_connection *c)
    : tim::service(name)
    , _d(new tim::p::a_inetd_service())
{
    assert(c);
    _d->_c = c;
}
