#pragma once

#include "tim_service.h"

#include <cstdint>


struct mg_mgr;

namespace tim
{

namespace p
{

struct mqtt_service;

}

class mqtt_service : public tim::service
{

public:

    mqtt_service(mg_mgr *mg,
                 std::uint16_t port = 1883,
                 const std::string &host = "localhost");
    ~mqtt_service();

private:


    std::unique_ptr<tim::p::mqtt_service> _d;
};

}
