#pragma once

#include <cstddef>


namespace tim
{

class mqtt_client;

namespace p
{

/**
 * Внутреннее состояние tim::mqtt_subscription (PIMPL).
 */
struct mqtt_subscription
{
    /** MQTT-клиент, выдавший подписку. */
    tim::mqtt_client *_client = nullptr;
    /** Идентификатор подписки внутри клиента. */
    std::size_t _id = 0;
};

}

}
