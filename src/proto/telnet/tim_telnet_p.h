#pragma once

#include "tim_telnet.h"

#include <cassert>


struct telnet_t;
union telnet_event_t;

namespace tim::p
{

struct telnet
{
    explicit telnet(tim::telnet *q)
        : _q(q)
    {
        assert(_q);
    }

    static void event_handler(telnet_t *telnet, telnet_event_t *event, void *data);

    tim::telnet *const _q;

    telnet_t *_telnet = nullptr;
    tim::telnet::input_handler _input_handler;
    std::string _term_name;
    unsigned _cols = 0;
    unsigned _rows = 0;
};

}
