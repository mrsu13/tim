#pragma once

#include "tim_a_inetd_service.h"


namespace tim
{

class mqtt_client;

namespace p
{

struct prompt_service;

}

class prompt_service : public tim::a_inetd_service
{

public:

    prompt_service(mg_connection *c, tim::mqtt_client &mqtt);
    ~prompt_service();

private:

    std::unique_ptr<tim::p::prompt_service> _d;
};

}
