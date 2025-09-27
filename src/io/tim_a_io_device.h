#pragma once

#include "tim_signal.h"

#include <cstddef>
#include <memory>
#include <string>


struct mg_connection;

namespace tim
{

namespace p
{

struct a_io_device;

}

class a_io_device
{

public:

    tim::signal<> ready_read;

    virtual ~a_io_device();

    mg_connection *connection() const;
    void close();

    std::size_t read(const char **data);
    bool write(const char *data, std::size_t size);
    bool write_str(const std::string &s);

protected:

    a_io_device(mg_connection *c);

private:

    std::unique_ptr<tim::p::a_io_device> _d;
};

}
