#pragma once

#include "tim_service.h"

#include <cstdint>


struct mg_connection;
struct mg_mgr;

namespace tim
{

namespace p
{

struct inetd;

}

class a_inetd_service;

class inetd : public tim::service
{

public:

    using service_factory = std::function<std::unique_ptr<tim::a_inetd_service>(mg_connection *c)>;

    ~inetd();

private:

    inetd(mg_mgr *mg, std::uint16_t port, service_factory factory);

    std::unique_ptr<tim::p::inetd> _d;
};

}
