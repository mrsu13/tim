#pragma once

#include "tim_uuid.h"


namespace tim::p
{

struct user
{
    tim::uuid _id;
    std::string _pkey;
    std::string _nick;
    std::string _icon;
    std::string _motto;
};

}
