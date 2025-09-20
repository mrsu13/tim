#pragma once

#include <stdbool.h>

typedef struct tim_inetd_service tim_inetd_service_t;

struct mg_connection;

void tim_inetd_service_init(tim_inetd_service_t *srv, const char *name, struct mg_connection *c);
void tim_inetd_service_destroy(tim_inetd_service_t *srv);

bool tim_inetd_service_read(tim_inetd_service_t *srv);
