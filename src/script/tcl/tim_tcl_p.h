#pragma once

#include "tim_uuid.h"

#include <cassert>
#include <string>


typedef struct _lil_t *lil_t;

namespace tim
{

class mqtt_client;
class tcl;

namespace p
{

struct tcl
{
    tcl(tim::tcl *q, tim::mqtt_client &mqtt)
        : _q(q)
        , _mqtt(mqtt)
    {
        assert(_q);
    }

    static void write(lil_t lil, const char *msg);
    static void dispatch(lil_t lil);

    tim::tcl *const _q;
    tim::mqtt_client &_mqtt;

    lil_t _lil = nullptr;
    tim::uuid _user_id;
    tim::uuid _last_post_id;
    bool _evaluating = false;
    std::string _prompt = "► ";
    std::string _error_msg;
    std::size_t _error_pos = 0;
};

}

}
