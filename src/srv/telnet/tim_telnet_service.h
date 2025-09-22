#pragma once

typedef struct tim_telnet_service tim_telnet_service_t;

struct mg_connection;

void tim_telnet_service_init(tim_telnet_service_t *srv, struct mg_connection *c);
void tim_telnet_service_destroy(tim_telnet_service_t *srv);
