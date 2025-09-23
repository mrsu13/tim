#pragma once

#include "tim_inetd.h"

#include <unordered_map>


struct mg_connection;

namespace tim
{

class a_inetd_service;

namespace p
{

struct inetd
{
    static void handle_events(mg_connection *c, int ev, void *ev_data);

    std::uint16_t _port = 0;
    mg_connection *_server = nullptr;

    tim::inetd::service_factory _factory;

    using connection_map = std::unordered_map<mg_connection *, std::unique_ptr<tim::a_inetd_service>>;
    connection_map _connections;
};

}

}
