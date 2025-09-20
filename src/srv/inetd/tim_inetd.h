#pragma once

#include "tim_service.h"


typedef struct tim_inetd tim_inetd_t;
typedef struct tim_inetd_service tim_inetd_service_t;
typedef tim_inetd_service_t *(*tim_service_factory_t)();

tim_inetd_t *tim_inetd_new(uint16_t port, tim_service_factory_t factory);
void tim_inetd_free(tim_inetd_t *srv);
