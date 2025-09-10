#include "tim_app.h"

#include "tim_app_p.h"
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

    return app;
}

void tim_app_free(tim_app_t *app)
{
    free(app);
}

void tim_app_exec(tim_app_t *app)
{
    assert(app);

    while (!app->quit)
    {
    }
}

void tim_app_quit(tim_app_t *app)
{
    assert(app);

    app->quit = true;
}
