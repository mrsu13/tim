#pragma once

#include "tim_telnet_server.h"

#include <cassert>
#include <string>


struct telnet_t;
union telnet_event_t;

namespace tim::p
{

struct telnet_server
{
    explicit telnet_server(tim::telnet_server *q)
        : _q(q)
    {
        assert(_q);
    }

    static void event_handler(telnet_t *telnet, telnet_event_t *event, void *data);

    tim::telnet_server *const _q;

    telnet_t *_telnet = nullptr;
    std::string _term_name;
    unsigned _cols = 0;
    unsigned _rows = 0;
};

}
