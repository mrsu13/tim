#pragma once

#include "tim_a_inetd_service.h"

#include <cstdarg>


struct mg_connection;

namespace tim
{

namespace p
{

struct a_telnet_service;

}

class a_telnet_service : public tim::a_inetd_service
{

public:

    ~a_telnet_service();

    std::size_t cols() const;
    void clear();

    int vprintf(const char *format, va_list args);

    int printf(const char *format, ... )
               __attribute__ ((format(printf, 2, 3)));

    virtual bool process_data(const char *data, std::size_t size) = 0;

protected:

    explicit a_telnet_service(const std::string &name, mg_connection *c);

private:

    bool ready_read(const char *data, std::size_t size, std::size_t *bytes_read = nullptr);
    bool ready_write(const char *data, std::size_t size, std::size_t *bytes_written = nullptr);

    std::unique_ptr<tim::p::a_telnet_service> _d;
};

}
