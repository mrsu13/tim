#pragma once

#include "tim_inetd.h"

#include <cstdint>
#include <functional>
#include <memory>


struct mg_connection;
struct mg_mgr;

namespace tim
{

namespace p
{

struct tcp_inetd;

}

class a_inetd_service;

// inetd поверх mongoose: слушает TCP-порт, на каждый mg_accept создаёт
// сервис через переданную фабрику.
class tcp_inetd : public tim::inetd
{

public:

    using service_factory = std::function<std::unique_ptr<tim::a_inetd_service>(mg_connection *c)>;

    ~tcp_inetd();

    static std::unique_ptr<tim::tcp_inetd> start(mg_mgr *mg,
                                                 std::uint16_t port,
                                                 service_factory factory,
                                                 bool tls_enabled = true,
                                                 const std::string &if_addr = "");

private:

    tcp_inetd(mg_mgr *mg,
              std::uint16_t port,
              bool tls_enabled,
              const std::string &if_addr,
              service_factory factory);

    std::unique_ptr<tim::p::tcp_inetd> _d;
};

}
