#pragma once

#include "tim_a_slot.h"

#include <cassert>
#include <functional>


namespace tim
{

template<typename R, typename... Args>
class slot : public a_slot<Args...>
{

public:

    slot(std::function<R (Args...)> fn)
        : _fn(fn)
    {
        assert(_fn);
    }

    void invoke(Args... args) const override
    {
        _fn(args...);
    }

private:

    std::function<R (Args...)> _fn;
};

}
