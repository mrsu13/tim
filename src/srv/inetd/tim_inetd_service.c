#include "tim_inetd_service.h"

#include "tim_inetd_service_p.h"

#include "tim_service.h"
#include "tim_trace.h"

#include "mongoose.h"

#include <assert.h>
#include <string.h>


void tim_inetd_service_init(tim_inetd_service_t *srv, const char *name, struct mg_connection *c)
{
    assert(srv);
    assert(c);

    memset(srv, 0, sizeof(tim_inetd_service_t));
    tim_service_init(&srv->super, name);

    srv->c = c;
}

void tim_inetd_service_destroy(tim_inetd_service_t *srv)
{
    if (srv->c)
        srv->c->is_draining = 1;

    tim_service_destroy(&srv->super);
}

struct mg_connection *tim_inetd_service_connection(tim_inetd_service_t *srv)
{
    assert(srv);

    return srv->c;
}

bool tim_inetd_service_read(tim_inetd_service_t *srv)
{
    assert(srv);
    assert(srv->ready_read && "ready_read is not set.");

    struct mg_iobuf *r = &(srv->c->recv);
    TIM_TRACE(Debug, "%s service got data: %.*s", tim_service_name(&srv->super), (int)r->len, r->buf);
    size_t read = 0;
    if (!srv->ready_read(srv, (const char *)r->buf, r->len, &read))
        return false;

    r->len -= read; // Tell Mongoose we've consumed data.
    return true;
}

bool tim_inetd_service_write(tim_inetd_service_t *srv, const char *data, size_t size, size_t *written)
{
    assert(srv);
    assert(data);
    assert(srv->ready_write && "ready_write is not set.");

    size_t n = 0;
    if (!srv->ready_write(srv, data, size, &n))
        return false;
    if (written)
        *written = n;
    return true;
}

bool tim_inetd_service_write_str(tim_inetd_service_t *srv, const char *s)
{
    assert(s);
    return tim_inetd_service_write(srv, s, strlen(s), NULL);
}
