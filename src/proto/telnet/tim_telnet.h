#pragma once

#include "tim_signal.h"

#include <memory>


namespace tim
{

class a_io_device;

namespace p
{

struct telnet;

}

class telnet
{

public:

    tim::signal<const char * /* data */, std::size_t /* size */> ready_read;

    telnet(tim::a_io_device *io);
    ~telnet();

    const std::string &terminal_name() const;
    std::size_t rows() const;
    std::size_t cols() const;

    bool write(const char *data, std::size_t size);

private:

    std::unique_ptr<tim::p::telnet> _d;
};

}
