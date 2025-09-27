#pragma once

#include "tim_non_copyable.h"

#include <cstddef>
#include <memory>
#include <utility>


namespace tim
{

class a_signal;

namespace p
{

struct signal_connection;

}

class signal_connection : private tim::non_copyable
{

public:

    signal_connection(const std::pair<tim::a_signal *, std::size_t> &s_id);
    ~signal_connection();

    bool connected() const;
    void disconnect();

private:

    std::unique_ptr<tim::p::signal_connection> _d;
};

}
