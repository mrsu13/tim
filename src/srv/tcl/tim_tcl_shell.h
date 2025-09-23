#pragma once

typedef struct tim_tcl_shell tim_tcl_shell_t;

struct mg_connection;

tim_tcl_shell_t *tim_tcl_shell_new(struct mg_connection *c);
void tim_tcl_shell_free(tim_tcl_shell_t *srv);
