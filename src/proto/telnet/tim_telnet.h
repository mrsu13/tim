#pragma once

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

    using input_handler = std::function<bool (const char *data, std::size_t size)>;

    telnet(tim::a_io_device *io, input_handler ih);
    ~telnet();

    const std::string &terminal_name() const;
    std::size_t rows() const;
    std::size_t cols() const;

    bool write(const char *data, std::size_t size) override;

protected:

    std::size_t ready_read(const char *data, std::size_t size) override;

private:

    std::unique_ptr<tim::p::telnet> _d;
};

}
