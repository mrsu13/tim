#pragma once

#include <cstdint>
#include <string>


struct mg_connection;

namespace tim::p
{

struct mqtt_service
{
    static void handle_events(mg_connection *c, int ev, void *ev_data);

    std::string _host;
    std::uint16_t _port = 0;
    mg_connection *_client = nullptr;
};

}
