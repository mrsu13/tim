#pragma once

#include "tim_signal.h"

#include <memory>


namespace tim
{

class a_io_device;

namespace p
{

struct a_protocol;

}

class a_protocol
{

public:

    tim::signal<const char * /* data */, std::size_t /* size */> data_ready;

    a_protocol(tim::a_io_device *io);
    virtual ~a_protocol();

    tim::a_io_device *io() const;

    virtual bool write(const char *data, std::size_t size) = 0;
    bool write_str(const std::string &s);

    virtual void process_raw_data(const char *data, std::size_t size) = 0;

private:

    std::unique_ptr<tim::p::a_protocol> _d;
};

}
