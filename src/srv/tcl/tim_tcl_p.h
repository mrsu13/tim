#pragma once

#include <cassert>
#include <string>


typedef struct _lil_t *lil_t;

namespace tim
{

class a_telnet_service;
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

    tim::a_telnet_service *_telnet = nullptr;
    lil_t _lil = nullptr;
    bool _evaluating = false;
    std::string _prompt = "► ";
    std::string _error_msg;
    std::size_t _error_pos = 0;
};

}

}
