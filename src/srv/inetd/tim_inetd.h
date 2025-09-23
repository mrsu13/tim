#pragma once

#include "tim_service.h"

#include <cstdint>
#include <functional>


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

    template<class S>
    inline static std::unique_ptr<tim::inetd> start(mg_mgr *mg, std::uint16_t port);

private:

    inetd(mg_mgr *mg, std::uint16_t port, service_factory factory);

    std::unique_ptr<tim::p::inetd> _d;
};

}


// Implementation

// Public

template<class S>
std::unique_ptr<tim::inetd> tim::inetd::start(mg_mgr *mg, std::uint16_t port)
{
    static_assert(std::is_base_of_v<tim::a_inetd_service, S>,
                  "S must be a descendant of tim::a_inetd_service class.");

    return std::unique_ptr<tim::inetd>(new tim::inetd(mg, port,
                                                      [](mg_connection *c)
                                                      {
                                                            return std::make_unique<S>(c);
                                                      }));
}
