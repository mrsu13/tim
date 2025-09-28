#pragma once

#include <cstddef>


namespace tim
{

class a_signal;

namespace p
{

struct signal_connection
{
    tim::a_signal *_signal = nullptr;
    std::size_t _connection_id = 0;
};

}

}
