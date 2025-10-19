#pragma once

#include "tim_service.h"


namespace tim
{

namespace p
{

struct post_service;

}

class post_service : public tim::service
{

public:

    post_service();
    ~post_service();

private:

    std::unique_ptr<tim::p::post_service> _d;
};

}
