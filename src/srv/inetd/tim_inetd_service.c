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
    tim_service_destroy(&srv->super);
}

bool tim_inetd_service_read(tim_inetd_service_t *srv)
{
    assert(srv);
    assert(srv->read);

    struct mg_iobuf *r = &(srv->c->recv);
    TIM_TRACE(Debug, "%s service got data: %.*s", tim_service_name(&srv->super), (int)r->len, r->buf);
    size_t read = 0;
    if (!srv->read((const char *)r->buf, r->len, &read))
        return false;

    r->len -= read; // Tell Mongoose we've consumed data.
    return true;
}
