#pragma once

#include <cstddef>


namespace tim
{

class mqtt_client;

namespace p
{

struct mqtt_subscription
{
    tim::mqtt_client *_client = nullptr;
    std::size_t _id = 0;
};

}

}
