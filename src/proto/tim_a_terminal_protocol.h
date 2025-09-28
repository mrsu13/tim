#pragma once

#include "tim_a_protocol.h"


namespace tim
{

class a_terminal_protocol : public tim::a_protocol
{

public:

    explicit a_terminal_protocol(tim::a_io_device *io);

    virtual const std::string &terminal_name() const = 0;
    virtual std::size_t rows() const = 0;
    virtual std::size_t cols() const = 0;
};

}
