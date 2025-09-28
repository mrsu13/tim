#pragma once

#include <cassert>


namespace tim
{

class a_terminal_protocol;
class vt;

namespace p
{

struct vt
{
    explicit vt(tim::vt *q)
        : _q(q)
    {
        assert(_q);
    }

    tim::vt *const _q;

    tim::a_terminal_protocol *_term_proto = nullptr;
};

}

}
