#pragma once

#include "tim_uuid.h"


namespace tim
{

struct user
{
    std::string title() const;

    tim::uuid id;
    std::string pub_key;
    std::string nick;
    std::string icon;
    std::string motto;
};

}
