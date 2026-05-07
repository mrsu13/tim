#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_signal_connection.h"

#include <cstddef>


namespace tim
{

class mqtt_client;
class mqtt_topic;
class sqlite_db;

namespace p
{

struct post_service
{
    post_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
        : _mqtt(mqtt), _db(db)
    {
    }

    void subscribe();
    void on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    tim::mqtt_client &_mqtt;
    tim::sqlite_db &_db;

    tim::signal_connection _on_connected;
    tim::mqtt_subscription _sub_post;
};
}

}
