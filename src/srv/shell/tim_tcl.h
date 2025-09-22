#pragma once

#include <stdbool.h>
#include <stddef.h>


typedef struct _lil_t *lil_t;
typedef struct tim_tcl tim_tcl_t;
typedef void (*tim_tcl_write_t)(lil_t lil, const char *msg);
typedef void (*tim_tcl_dispatch_t)(lil_t lil);

tim_tcl_t *tim_tcl_new(tim_tcl_write_t write, tim_tcl_dispatch_t dispatch);
void tim_tcl_free(tim_tcl_t *tcl);

bool tim_tcl_evaluating(const tim_tcl_t *tcl);
bool tim_tcl_eval(tim_tcl_t *tcl, const char *program, const char **res, void *user_data);
void tim_tcl_break_eval(tim_tcl_t *tcl);

const char *tim_tcl_prompt(const tim_tcl_t *tcl);
const char *tim_tcl_error_msg(const tim_tcl_t *tcl);
size_t tim_tcl_error_pos(const tim_tcl_t *tcl);
