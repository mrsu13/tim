#include "tim_inetd.h"

#include "tim_inetd_service.h"
#include "tim_trace.h"

#include "tim_service_p.h"

#include "mongoose.h"
#include "uthash.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct tim_inetd_connection
{
    struct mg_connection *c;
    tim_inetd_service_t *service;
    UT_hash_handle hh; // Makes this structure hashable.
} tim_inetd_connection_t;

typedef struct tim_inetd
{
    tim_service_t super;
    uint16_t port;
    struct mg_connection *server;
    tim_service_factory_t factory;
    tim_inetd_connection_t *connections;
} tim_inetd_t;

static tim_inetd_connection_t *tim_find_connection(tim_inetd_t *srv, struct mg_connection *c);
static void tim_inetd_handle_events(struct mg_connection *c, int ev, void *ev_data);

tim_inetd_t *tim_inetd_new(struct mg_mgr *mg, uint16_t port, tim_service_factory_t factory)
{
    assert(mg);
    assert(port > 1024 && "Port number must be greater than 1024.");
    assert(factory);

    tim_inetd_t *srv = (tim_inetd_t *)calloc(1, sizeof(tim_inetd_t));
    assert(srv);

    tim_service_init(&srv->super, "inetd");

    srv->port = port;
    srv->factory = factory;

    char url[128];
    snprintf(url, sizeof(url), "tcp://0.0.0.0:%u", srv->port);
    if (!(srv->server = mg_listen(mg, url, tim_inetd_handle_events, srv)))
    {
        TIM_TRACE(Error, "Failed to create inetd at port %u.", srv->port);
        return NULL;
    }

    return srv;
}

void tim_inetd_free(tim_inetd_t *srv)
{
    tim_service_destroy(&srv->super);
    free(srv);
}


// Static

tim_inetd_connection_t *tim_find_connection(tim_inetd_t *srv, struct mg_connection *c)
{
    assert(srv);
    assert(c);
    tim_inetd_connection_t *conn;
    HASH_FIND_PTR(srv->connections, c, conn);
    assert(conn);
    return conn;
}

void tim_inetd_handle_events(struct mg_connection *c, int ev, void *ev_data)
{
    tim_inetd_t *self = (tim_inetd_t *)c->fn_data;
    assert(self);

    switch (ev)
    {
        case MG_EV_OPEN:
            if (c->is_listening == 1)
                TIM_TRACE(Info, "inetd is listening at port %u.", self->port);
            break;

        case MG_EV_ACCEPT:
            TIM_TRACE(Info, "inetd accepted a connection at port %u.", self->port);
            /* For the future support of TLS.
            if (mg_url_is_ssl(s_lsn))
            {
                struct mg_tls_opts opts =
                {
                    .ca = mg_unpacked("/certs/ss_ca.pem"),
                    .cert = mg_unpacked("/certs/ss_server.pem"),
                    .key = mg_unpacked("/certs/ss_server.pem")
                };
                mg_tls_init(c, &opts);
            } */

#ifndef NDEBUG
            c->is_hexdumping = 1;
#endif

            tim_inetd_service_t *srv = self->factory(c);
            if (srv)
            {
                tim_inetd_connection_t *conn = (tim_inetd_connection_t *)calloc(1, sizeof(tim_inetd_connection_t));
                assert(conn);
                conn->c = c;
                conn->service = srv;
                HASH_ADD_PTR(self->connections, c, conn);
            }
            else
            {
                TIM_TRACE(Error, "Failed to create inetd service at port %u.", self->port);
                c->is_draining = 1;
            }
            break;

        case MG_EV_READ:
            if (!c->is_draining
                    && !tim_inetd_service_read(tim_find_connection(self, c)->service))
                c->is_draining = 1;
            break;

        case MG_EV_CLOSE:
        {
            TIM_TRACE(Debug, "inetd connection at port %u disconnected.", self->port);
            tim_inetd_connection_t *conn = tim_find_connection(self, c);
            HASH_DEL(self->connections, conn);
            free(conn);
            break;
        }

        case MG_EV_ERROR:
            if (!c->is_draining)
            {
                TIM_TRACE(Error, "Error: %s", (char *)ev_data);
                tim_inetd_connection_t *conn = tim_find_connection(self, c);
                HASH_DEL(self->connections, conn);
                free(conn);
            }
            break;
    }
}
