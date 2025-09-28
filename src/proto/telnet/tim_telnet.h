#pragma once

#include "tim_a_protocol.h"

#include <memory>


namespace tim
{

namespace p
{

struct telnet;

}

class telnet : public tim::a_protocol
{

public:

    telnet(tim::a_io_device *io);
    ~telnet();

    const std::string &terminal_name() const;
    std::size_t rows() const;
    std::size_t cols() const;

    bool write(const char *data, std::size_t size) override;
    void process_raw_data(const char *data, std::size_t size) override;

private:

    std::unique_ptr<tim::p::telnet> _d;
};

}
