#pragma once

#include "tim_inetd_service.h"


struct mg_connection;

namespace tim
{

namespace p
{

struct a_telnet_service;

}

class a_telnet_service : public tim::inetd_service
{

public:

    ~a_telnet_service();

protected:

    explicit a_telnet_service(mg_connection *c);

    virtual bool process_data(const char *data, std::size_t size) = 0;

private:

    bool ready_read(const char *data, std::size_t size, std::size_t *bytes_read = nullptr);
    bool ready_write(const char *data, std::size_t size, std::size_t *bytes_written = nullptr);

    std::unique_ptr<tim::p::telnet_service> _d;
};

}
