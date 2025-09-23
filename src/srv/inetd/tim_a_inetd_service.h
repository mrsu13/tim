#pragma once

#include "tim_service.h"

#include <cstddef>

struct mg_connection;


namespace tim
{

namespace p
{

struct a_inetd_service;

}

class a_inetd_service : public tim::service
{

public:

    ~a_inetd_service();

    mg_connection *connection() const;
    void close();

    bool read();
    bool write(const char *data, std::size_t size, std::size_t *bytes_written = nullptr);
    bool write_str(const std::string &s);

protected:

    a_inetd_service(const std::string &name, mg_connection *c);

    virtual bool ready_read(const char *data, std::size_t size, std::size_t *bytes_read = nullptr) = 0;
    virtual bool ready_write(const char *data, std::size_t size, std::size_t *bytes_written = nullptr) = 0;

private:

    std::unique_ptr<tim::p::a_inetd_service> _d;
};

}
