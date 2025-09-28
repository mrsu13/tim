#pragma once

#include "tim_a_inetd_service.h"


namespace tim
{
    
namespace p
{

struct prompt_service;

}

class prompt_service : public tim::a_inetd_service
{

public:

    explicit prompt_service(mg_connection *c);
    ~prompt_service();

private:

    std::unique_ptr<tim::p::prompt_service> _d;
};

}
