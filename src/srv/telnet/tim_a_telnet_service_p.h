#pragma once

#include <cassert>


struct telnet_t;
union telnet_event_t;

namespace tim
{

class a_telnet_service;

namespace p
{

struct a_telnet_service
{
    explicit a_telnet_service(tim::a_telnet_service *q)
        : _q(q)
    {
        assert(_q);
    }

    static void event_handler(telnet_t *telnet, telnet_event_t *event, void *data);

    tim::a_telnet_service *const _q;
    telnet_t *_telnet = nullptr;
};

}

}
