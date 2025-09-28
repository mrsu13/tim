#pragma once

#include "tim_a_terminal_protocol.h"

#include <memory>


namespace tim
{

namespace p
{

struct telnet_server;

}

class telnet_server : public tim::a_terminal_protocol
{

public:

    telnet_server(tim::a_io_device *io);
    ~telnet_server();

    const std::string &terminal_name() const override;
    std::size_t rows() const override;
    std::size_t cols() const override;

    bool write(const char *data, std::size_t size) override;
    void process_raw_data(const char *data, std::size_t size) override;

private:

    std::unique_ptr<tim::p::telnet_server> _d;
};

}
