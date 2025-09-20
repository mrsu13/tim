#pragma once

#include "tim_service_p.h"

struct mg_connection;

typedef struct tim_inetd_service
{
    tim_service_t super;
    struct mg_connection *c;
} tim_inetd_service_t;
