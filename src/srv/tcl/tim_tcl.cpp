#include "tim_tcl.h"

#include "tim::tcl::p.h"

#include "lil.hpp"
#include "utf8.h"

#include <cassert>


// Public

tim::tcl::tcl()
    : _d(new tim::p::tcl())
{
    _d->_lil = lil_new();

    lil_callback(_d->_lil, LIL_CALLBACK_WRITE, (lil_callback_proc_t)tim::p::tcl::write);
    lil_callback(_d->_lil, LIL_CALLBACK_DISPATCH, (lil_callback_proc_t)tim::p::tcl::dispatch);
}

tim::tcl::~tcl()
{
    lil_free(_d->_lil);
}

bool tim::tcl::evaluating() const
{
    return _d->_evaluating;
}

bool tim::tcl::eval(const std::string &program, std::string *res, void *user_data)
{
    assert(!_d->_evaluating && "Recursive evaluating is not allowed.");

    if (program.empty())
    {
        if (res)
            res->clear();
        return true;
    }

    _d->_evaluating = true;

    void *old_data = lil_get_data(_d->_lil);
    lil_set_data(_d->_lil, user_data);

    _d->_error_msg = nullptr;
    _d->_error_pos = 0;

    lil_value_t rv = lil_parse(_d->_lil, program.c_str(), program.size(), 1);
    const bool ok = !lil_error(_d->_lil, &_d->_error_msg, &_d->_error_pos);
    _d->_error_pos = utf8nlen((const utf8_int8_t *)program.c_str(), _d->_error_pos);
    if (ok
            && res)
        *res = lil_to_string(rv);

    lil_free_value(rv);
    lil_set_data(_d->_lil, old_data);

    _d->_evaluating = false;

    return ok;
}

void tim::tcl::break_eval()
{
    lil_break_run(_d->_lil, true);
}

const std::string &tim::tcl::prompt() const
{
    return _d->_prompt;
}

const char *tim::tcl::error_msg() const
{
    return _d->_error_msg;
}

std::size_t tim::tcl::error_pos() const
{
    return _d->_error_pos;
}


// Private

void tim::p::tcl::write(lil_t lil, const char *msg)
{

}

void tim::p::tcl::dispatch(lil_t lil)
{

}
