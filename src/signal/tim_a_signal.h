#pragma once

#include <cstddef>


namespace tim
{

class a_signal
{

public:

    virtual ~a_signal() = default;

    virtual bool disconnect(std::size_t connection_id) = 0;
};

}
