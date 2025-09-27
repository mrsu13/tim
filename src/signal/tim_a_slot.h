#pragma once

namespace tim
{

template<typename... Args>
class a_slot
{

public:

    virtual ~a_slot() = default;

    virtual void invoke(Args... args) const = 0;
};

}
