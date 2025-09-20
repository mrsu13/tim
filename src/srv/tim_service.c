#include "tim_service.h"

#include "tim_service_p.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


void tim_service_init(tim_service_t *srv, const char *name)
{
    assert(srv);
    assert(name && *name && "Service name must not be empty.");

    memset(srv, 0, sizeof(tim_service_t));

    static uint64_t next_id = 1;
    srv->id = next_id++;
    srv->name = strdup(name);
}

void tim_service_destroy(tim_service_t *srv)
{
    assert(srv);
    free(srv->name);
    free(srv);
}

uint64_t tim_service_id(const tim_service_t *srv)
{
    assert(srv);

    return srv->id;
}

const char *tim_service_name(const tim_service_t *srv)
{
    assert(srv);

    return srv->name;
}
