#include "tim_app.h"

#include "tim_app_p.h"

#include "tim_config.h"
#include "tim_inetd.h"
#include "tim_tcl_shell.h"
#include "tim_trace.h"
#include "tim_version.h"

#include <cassert>


// Public

tim::app::app(int argc, char **argv)
    : _d(new tim::p::app())
{
    (void) argc;
    (void) argv;

    // mg_log_set(MG_LL_VERBOSE);
    mg_mgr_init(&_d->_mg);

    _d->_tcl_shell_inetd = tim::inetd::start<tim::tcl_shell>(&_d->_mg, tim::CLIENT_PORT);
}

tim::app::~app()
{
    mg_mgr_free(&_d->_mg);
}

const std::string &tim::app::name()
{
    return tim::p::app::name();
}

void tim::app::set_name(const std::string &name)
{
    tim::p::app::name() = name;
}

const std::string &tim::app::org_name()
{
    return tim::p::app::org_name();
}

void tim::app::set_org_name(const std::string &name)
{
    tim::p::app::org_name() = name;
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
