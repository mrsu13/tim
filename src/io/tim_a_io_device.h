#pragma once

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

    using data_handler = std::function<std::size_t (const char *data, std::size_t size)>;

    virtual ~a_io_device();

    mg_connection *connection() const;
    void close();

    void read();
    bool write(const char *data, std::size_t size);
    bool write_str(const std::string &s);

protected:

    a_io_device(mg_connection *c, data_handler dh);

private:

    std::unique_ptr<tim::p::a_io_device> _d;
};

}
