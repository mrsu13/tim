#pragma once

#include "tim_mqtt_client.h"

#include <filesystem>
#include <utility>
#include <vector>


struct mg_connection;
struct mg_timer;

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
    static void ping(void *data);

    tim::mqtt_client *const _q;

    mg_mgr *_mg = nullptr;
    std::filesystem::path _url;
    mg_connection *_client = nullptr;
    mg_timer *_timer = nullptr;
    bool _connected = false;

    using subscribers = std::vector<std::pair<std::filesystem::path, tim::mqtt_client::message_handler>>;
    subscribers _subscribers;
};

}
