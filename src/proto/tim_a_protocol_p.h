#pragma once

#include <cassert>


namespace tim
{

class a_io_device;
class a_protocol;

namespace p
{

struct a_protocol
{
    explicit a_protocol(tim::a_protocol *q)
        : _q(q)
    {
        assert(_q);
    }

    void on_ready_read();

    tim::a_protocol *const _q;
    tim::a_io_device *_io = nullptr;
};

}

}
