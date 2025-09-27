#pragma once

#include <cassert>


namespace tim
{

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
    unsigned _cols = 0;
    unsigned _rows = 0;
};

}

}
