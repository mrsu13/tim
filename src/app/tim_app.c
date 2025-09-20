#include "tim_app.h"

#include "tim_app_p.h"
#include "tim_config.h"
#include "tim_inetd.h"
#include "tim_trace.h"
#include "tim_version.h"

#include <assert.h>
#include <stdlib.h>


// Public

tim_app_t *tim_app_new(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    tim_app_t *app = (tim_app_t *)calloc(1, sizeof(tim_app_t));
    assert(app && "Failed to create application object.");

    app->quit = false;

    mg_log_set(MG_LL_VERBOSE);
    mg_mgr_init(&app->mg);

    return app;
}

void tim_app_free(tim_app_t *app)
{
    assert(app);

    mg_mgr_free(&app->mg);
    free(app);
}

void tim_app_exec(tim_app_t *app)
{
    assert(app);

    while (!app->quit)
        mg_mgr_poll(&app->mg, 1000 /* 1 sec */);
}

void tim_app_quit(tim_app_t *app)
{
    assert(app);

    app->quit = true;
}

struct mg_mgr *tim_app_mongoose(tim_app_t *app)
{
    assert(app);

    return &app->mg;
}
