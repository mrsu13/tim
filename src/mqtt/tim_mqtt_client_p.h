#pragma once

#include "tim_mqtt_client.h"

#include <unordered_map>


struct mg_connection;

namespace tim::p
{

struct mqtt_client
{
    explicit mqtt_client(tim::mqtt_client *q)
        : _q(q)
    {
        assert(_q);
    }

    static void handle_events(mg_connection *c, int ev, void *ev_data);

    tim::mqtt_client *const _q;

    std::string _host;
    std::uint16_t _port = 0;
    bool _tls_enabled = true;
    mg_connection *_client = nullptr;

    using subscribers = std::unordered_multimap<std::string, tim::mqtt_client::message_handler>;
    subscribers _subscribers;
};

}
