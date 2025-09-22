#include "tim_telnet_service.h"

#include "tim_telnet_service_p.h"

#include "tim_inetd_service.h"
#include "tim_trace.h"

#include "libtelnet.h"
#include "mongoose.h"

#include <assert.h>
#include <stdlib.h>


static void tim_telnet_service_event_handler(struct telnet_t *telnet, union telnet_event_t *event, void *data);
static bool tim_telnet_service_ready_read(void *srv, const char *data, size_t size, size_t *read);
static bool tim_telnet_service_ready_write(void *srv, const char *data, size_t size, size_t *written);

void tim_telnet_service_init(tim_telnet_service_t *srv, struct mg_connection *c)
{
    assert(srv);

    memset(srv, 0, sizeof(tim_telnet_service_t));
    tim_inetd_service_init(&srv->super, "telnet", c);
    srv->super.ready_read = &tim_telnet_service_ready_read;
    srv->super.ready_write = &tim_telnet_service_ready_write;

    srv->telnet = telnet_init(NULL,
                              &tim_telnet_service_event_handler,
                              TELNET_FLAG_NVT_EOL, // Receive data with translation of the TELNET NVT CR NUL
                                                   // and CR LF sequences specified in RFC854 to C carriage
                                                   // return (\r) and C newline (\n), respectively.
                              srv);
    assert(srv->telnet);

    telnet_negotiate(srv->telnet, TELNET_DO, TELNET_TELOPT_NAWS);
    telnet_negotiate(srv->telnet, TELNET_DO, TELNET_TELOPT_TTYPE);
    telnet_negotiate(srv->telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
    telnet_negotiate(srv->telnet, TELNET_WILL, TELNET_TELOPT_SGA);
}

void tim_telnet_service_destroy(tim_telnet_service_t *srv)
{
    assert(srv);

    telnet_free(srv->telnet);
    srv->telnet = NULL;
    tim_inetd_service_destroy(&srv->super);
}


// Static

void tim_telnet_service_event_handler(struct telnet_t *telnet, union telnet_event_t *event, void *data)
{
    (void) telnet;

    tim_telnet_service_t *self = (tim_telnet_service_t *)data;
    assert(self);

    switch (event->type)
    {
        case TELNET_EV_DATA:
            assert(self->process_data && "process_data is not set.");
            if (!self->process_data(self, event->data.buffer, event->data.size))
                tim_inetd_service_close((tim_inetd_service_t *)self);
            break;

        case TELNET_EV_SEND:
            mg_send(self->super.c, event->data.buffer, event->data.size);
            break;

        case TELNET_EV_ERROR:
            TIM_TRACE(Error, "Telnet error: %s", event->error.msg);
            break;

        default:
            break;
    }
}

bool tim_telnet_service_ready_read(void *srv, const char *data, size_t size, size_t *read)
{
    tim_telnet_service_t *self = (tim_telnet_service_t *)srv;
    assert(self);
    assert(data);
    assert(read);

    telnet_recv(self->telnet, data, size);
    *read = size;
    return true;
}

bool tim_telnet_service_ready_write(void *srv, const char *data, size_t size, size_t *written)
{
    tim_telnet_service_t *self = (tim_telnet_service_t *)srv;
    assert(self);
    assert(data);

    telnet_send_text(self->telnet, data, size);
    if (written)
        *written = size;

    return true;
}
