#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_mqtt_topic.h"
#include "tim_signal_connection.h"
#include "tim_user.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>


namespace tim
{

class mqtt_client;
class prompt_service;
class prompt_shell;
class sqlite_db;
class ssh_terminal_protocol;
class tcl;
class vt;

namespace p
{

struct prompt_service
{
    prompt_service(tim::prompt_service *q, tim::mqtt_client &mqtt, tim::sqlite_db &db)
        : _q(q)
        , _mqtt(mqtt)
        , _db(db)
    {
        assert(_q);
    }

    void load_user_from_db();
    void subscribe();
    void on_data_ready(const char *data, std::size_t size);
    void on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    tim::prompt_service *const _q;
    tim::mqtt_client &_mqtt;
    tim::sqlite_db &_db;

    std::unique_ptr<tim::ssh_terminal_protocol> _proto;
    std::unique_ptr<tim::vt>                    _terminal;
    std::unique_ptr<tim::tcl>                   _tcl;
    std::unique_ptr<tim::prompt_shell>          _shell;
    tim::user                                   _user;
    tim::uuid                                   _last_seen_post;
    tim::uuid                                   _last_seen_post_author;
    // UUID сообщений, опубликованных именно из этой сессии. Используется
    // в on_post, чтобы не показывать пользователю эхо собственного ввода.
    // Фильтрация по publisher_id == _user.id не годится — у нескольких
    // одновременных сессий одного пользователя совпадает _user.id.
    std::unordered_set<tim::uuid>               _own_posts;

    tim::signal_connection                      _on_data_ready;
    tim::signal_connection                      _on_posted;
    tim::signal_connection                      _on_connected;
    tim::mqtt_subscription                      _sub_post;
    tim::mqtt_subscription                      _sub_self_setnick;
    tim::mqtt_subscription                      _sub_self_seticon;
};

}

}
