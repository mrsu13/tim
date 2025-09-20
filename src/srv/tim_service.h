#pragma once

typedef struct tim_service tim_service_t;

void tim_service_init(tim_service_t *srv, const char *name);
void tim_service_destroy(tim_service_t *srv);

const char *tim_service_name(const tim_service_t *srv);
