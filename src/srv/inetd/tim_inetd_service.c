#include "tim_inetd_service.h"

#include "tim_inetd_service_p.h"

#include "tim_service.h"

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
