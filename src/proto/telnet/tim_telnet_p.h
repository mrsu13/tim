#pragma once

#include "tim_telnet.h"

#include <cassert>
#include <string>


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
    std::string _term_name;
    unsigned _cols = 0;
    unsigned _rows = 0;
};

}
