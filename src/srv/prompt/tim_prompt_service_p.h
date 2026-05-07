#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_mqtt_topic.h"
#include "tim_signal_connection.h"
#include "tim_user.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>


namespace tim
{

class mqtt_client;
class prompt_service;
class prompt_shell;
class ssh_terminal_protocol;
class tcl;
class vt;

namespace p
{

struct prompt_service
{
    prompt_service(tim::prompt_service *q, tim::mqtt_client &mqtt)
        : _q(q)
        , _mqtt(mqtt)
    {
        assert(_q);
    }

    void subscribe();
    void on_data_ready(const char *data, std::size_t size);
    void on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    tim::prompt_service *const _q;
    tim::mqtt_client &_mqtt;

    std::unique_ptr<tim::ssh_terminal_protocol> _proto;
    std::unique_ptr<tim::vt>                    _terminal;
    std::unique_ptr<tim::tcl>                   _tcl;
    std::unique_ptr<tim::prompt_shell>          _shell;
    tim::mqtt_topic                             _topic;
    tim::user                                   _user;

    tim::signal_connection                      _on_data_ready;
    tim::signal_connection                      _on_posted;
    tim::signal_connection                      _on_connected;
    tim::mqtt_subscription                      _sub_post;
};

}

}
