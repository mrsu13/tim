#pragma once

#include "tim_inetd_service_p.h"


struct telnet_t;
union telnet_event_t;

typedef struct tim_telnet_service
{
    tim_inetd_service_t super;
    struct telnet_t *telnet;
    bool (*process_data)(void *srv, const char *data, size_t size);
} tim_telnet_service_t;
