#pragma once


typedef struct tim_app tim_app_t;

tim_app_t *tim_app_new(int argc, char **argv);
void tim_app_free(tim_app_t *app);

void tim_app_exec(tim_app_t *app);
void tim_app_quit(tim_app_t *app);
