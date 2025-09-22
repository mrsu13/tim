#pragma once

#include <stdbool.h>
#include <stddef.h>


typedef struct tim_inetd_service tim_inetd_service_t;

struct mg_connection;

void tim_inetd_service_init(tim_inetd_service_t *srv, const char *name, struct mg_connection *c);
void tim_inetd_service_destroy(tim_inetd_service_t *srv);

struct mg_connection *tim_inetd_service_connection(tim_inetd_service_t *srv);
void tim_inetd_service_close(tim_inetd_service_t *srv);

bool tim_inetd_service_read(tim_inetd_service_t *srv);
bool tim_inetd_service_write(tim_inetd_service_t *srv, const char *data, size_t size, size_t *written);
bool tim_inetd_service_write_str(tim_inetd_service_t *srv, const char *s);
