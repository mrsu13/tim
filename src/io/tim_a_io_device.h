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

    virtual ~a_io_device();

    mg_connection *connection() const;
    void close();

    bool read_from_connection();
    bool write(const char *data, std::size_t size, std::size_t *bytes_written = nullptr);
    bool write_str(const std::string &s);

protected:

    a_io_device(mg_connection *c);

    virtual bool ready_read(const char *data, std::size_t size, std::size_t *bytes_read = nullptr) = 0;
    virtual bool ready_write(const char *data, std::size_t size, std::size_t *bytes_written = nullptr) = 0;

private:

    std::unique_ptr<tim::p::a_io_device> _d;
};

}
