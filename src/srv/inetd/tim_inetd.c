#include "tim_inetd.h"

#include "tim_service_p.h"

#include <assert.h>
#include <stdlib.h>


typedef struct tim_inetd
{
    tim_service_t super;
    tim_service_factory_t factory;
} tim_inetd_t;

tim_inetd_t *tim_inetd_new(uint16_t port, tim_service_factory_t factory)
{
    assert(port > 1024 && "Port number must be greater than 1024.");
    assert(factory);

    tim_inetd_t *srv = (tim_inetd_t *)calloc(1, sizeof(tim_inetd_t));
    assert(srv);

    tim_service_init(&srv->super, "inetd");

    srv->factory = factory;

    return srv;
}

void tim_inetd_free(tim_inetd_t *srv)
{
    tim_service_destroy(&srv->super);
    free(srv);
}
