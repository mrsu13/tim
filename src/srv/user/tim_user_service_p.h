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

struct user_service
{
    user_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
        : _mqtt(mqtt), _db(db)
    {
    }

    void subscribe();
    void connect(const tim::mqtt_topic &topic, const char *data, std::size_t size);
    void setnick(const tim::mqtt_topic &topic, const char *data, std::size_t size);
    void seticon(const tim::mqtt_topic &topic, const char *data, std::size_t size);
    void setmotto(const tim::mqtt_topic &topic, const char *data, std::size_t size);
    void setpubkey(const tim::mqtt_topic &topic, const char *data, std::size_t size);
    void subscribe_to(const tim::mqtt_topic &topic, const char *data, std::size_t size);
    void unsubscribe_from(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    tim::mqtt_client &_mqtt;
    tim::sqlite_db &_db;

    tim::signal_connection _on_connected;
    tim::mqtt_subscription _sub_connect;
    tim::mqtt_subscription _sub_setnick;
    tim::mqtt_subscription _sub_seticon;
    tim::mqtt_subscription _sub_setmotto;
    tim::mqtt_subscription _sub_setpubkey;
    tim::mqtt_subscription _sub_subscribe;
    tim::mqtt_subscription _sub_unsubscribe;
};
}

}
