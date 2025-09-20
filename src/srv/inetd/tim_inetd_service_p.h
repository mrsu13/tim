#pragma once

#include "tim_service_p.h"

#include <stdbool.h>
#include <stddef.h>


struct mg_connection;

typedef struct tim_inetd_service
{
    tim_service_t super;
    struct mg_connection *c;
    bool (*read)(const char *data, size_t size, size_t *read);
} tim_inetd_service_t;
