#include "tim_tcl.h"

#include "lil.h"
#include "utf8.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


typedef struct tim_tcl
{
    lil_t lil;
    bool evaluating;
    const char *prompt;
    const char *error_msg;
    size_t error_pos;
} tim_tcl_t;

tim_tcl_t *tim_tcl_new(tim_tcl_write_t write, tim_tcl_dispatch_t dispatch)
{
    assert(write);

    tim_tcl_t *tcl = (tim_tcl_t *)calloc(1, sizeof(tim_tcl_t));
    assert(tcl);

    tcl->lil = lil_new();
    tcl->prompt = "► ";

    lil_callback(tcl->lil, LIL_CALLBACK_WRITE, (lil_callback_proc_t)write);
    if (dispatch)
        lil_callback(tcl->lil, LIL_CALLBACK_DISPATCH, (lil_callback_proc_t)dispatch);

    return tcl;
}

void tim_tcl_free(tim_tcl_t *tcl)
{
    assert(tcl);

    lil_free(tcl->lil);
    free(tcl);
}

bool tim_tcl_evaluating(const tim_tcl_t *tcl)
{
    assert(tcl);

    return tcl->evaluating;
}

bool tim_tcl_eval(tim_tcl_t *tcl, const char *program, const char **res, void *user_data)
{
    assert(tcl);
    assert(!tcl->evaluating && "Recursive evaluating is not allowed.");

    if (!program
            || !*program)
    {
        if (res)
            *res = NULL;
        return true;
    }

    tcl->evaluating = true;

    void *old_data = lil_get_data(tcl->lil);
    lil_set_data(tcl->lil, user_data);

    tcl->error_msg = NULL;
    tcl->error_pos = 0;

    lil_value_t rv = lil_parse(tcl->lil, program, strlen(program), 1);
    const bool ok = !lil_error(tcl->lil, &tcl->error_msg, &tcl->error_pos);
    tcl->error_pos = utf8nlen((const utf8_int8_t *)program, tcl->error_pos);
    if (ok
            && res)
        *res = lil_to_string(rv);

    lil_free_value(rv);
    lil_set_data(tcl->lil, old_data);

    tcl->evaluating = false;

    return ok;
}

void tim_tcl_break_eval(tim_tcl_t *tcl)
{
    assert(tcl);
    lil_break_run(tcl->lil, true);
}

const char *tim_tcl_prompt(const tim_tcl_t *tcl)
{
    assert(tcl);
    return tcl->prompt;
}

const char *tim_tcl_error_msg(const tim_tcl_t *tcl)
{
    assert(tcl);
    return tcl->error_msg;
}

size_t tim_tcl_error_pos(const tim_tcl_t *tcl)
{
    assert(tcl);
    return tcl->error_pos;
}
