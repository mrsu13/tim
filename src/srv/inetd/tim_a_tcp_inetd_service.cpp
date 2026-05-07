#include "tim_a_tcp_inetd_service.h"

#include "mongoose.h"

#include <cassert>


// Открытые

void tim::a_tcp_inetd_service::close()
{
    if (_c)
        _c->is_draining = 1;
}

std::size_t tim::a_tcp_inetd_service::read(const char **data)
{
    assert(data);
    assert(_c);

    struct mg_iobuf *r = &_c->recv;
    *data = (const char *)r->buf;
    const std::size_t size = r->len;
    r->len = 0; // Сообщаем mongoose, что данные считаны.
    return size;
}

bool tim::a_tcp_inetd_service::write(const char *data, std::size_t size)
{
    assert(data);
    assert(_c);

    return size
                ? mg_send(_c, data, size)
                : true;
}

mg_connection *tim::a_tcp_inetd_service::connection() const noexcept
{
    return _c;
}


// Защищённые

tim::a_tcp_inetd_service::a_tcp_inetd_service(const std::string &name, mg_connection *c)
    : tim::a_inetd_service(name)
    , _c(c)
{
    assert(_c);
}

tim::a_tcp_inetd_service::~a_tcp_inetd_service()
{
    if (_c)
        _c->is_draining = 1;
}
