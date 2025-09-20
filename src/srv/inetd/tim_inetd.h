#pragma once

#include "tim_service.h"

struct mg_connection;

typedef struct tim_inetd tim_inetd_t;
typedef struct tim_inetd_service tim_inetd_service_t;
typedef tim_inetd_service_t *(*tim_service_factory_t)(struct mg_connection *c);

struct mg_mgr;

tim_inetd_t *tim_inetd_new(struct mg_mgr *mg, uint16_t port, tim_service_factory_t factory);
void tim_inetd_free(tim_inetd_t *srv);
