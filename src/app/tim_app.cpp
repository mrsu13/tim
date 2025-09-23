#include "tim_app.h"

#include "tim::app::p.h"

#include "tim_config.h"
#include "tim_inetd.h"
#include "tim_tcl_shell.h"
#include "tim_trace.h"
#include "tim_version.h"

#include <cassert.h>


// Public

tim::app::app(int argc, char **argv)
    : _d(new tim::p::app())
{
    (void) argc;
    (void) argv;

    // mg_log_set(MG_LL_VERBOSE);
    mg_mgr_init(&_d->_mg);

    _d->_tcl_shell_inetd.reset(new tim::inetd<tim::tcl_shell>(&_d->_mg, TIM_CLIENT_PORT));
}

tim::app::~app()
{
    mg_mgr_free(&_d->_mg);
}

void tim::app::exec()
{
    while (!_d->_quit)
        mg_mgr_poll(&_d->_mg, 1000 /* 1 sec */);
}

void tim::app::quit()
{
    _d->_quit = true;
}

mg_mgr *tim::app::mongoose() const
{
    return &_d->_mg;
}
