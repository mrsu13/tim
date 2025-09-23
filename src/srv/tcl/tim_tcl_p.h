#pragma once

#include <cassert>
#include <string>


namespace tim
{

class tcl;

namespace p
{

struct tcl
{
    explicit tcl(tim::tcl *q)
        : _q(q)
    {
        assert(_q);
    }

    static void write(lil_t lil, const char *msg);
    static void dispatch(lil_t lil);

    tim::tcl *const _q;
    lil_t _lil = nullptr;
    bool _evaluating = false;
    std::string _prompt = "► ";
    const char *error_msg = nullptr;
    std::size_t error_pos = 0;
};

}

}
