#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_mqtt_topic.h"
#include "tim_signal_connection.h"
#include "tim_user.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
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

    // Загружает пользователя по UUID из БД (nick, icon). Если строки нет,
    // возвращает user с одним лишь id; nick/icon остаются пустыми.
    tim::user load_user(const tim::uuid &id);
    // Возвращает пользователя из локального кэша; при первом обращении
    // подтягивает запись из БД. Для собственного UUID возвращает _user.
    tim::user user_for(const tim::uuid &id);
    // Подтягивает текущие подписки пользователя из БД в локальный кэш.
    void load_subscriptions();
    // Печатает в терминал последние N сообщений из БД, чтобы при входе
    // в чат пользователь сразу видел контекст разговора.
    void load_post_history();

    void subscribe();
    void on_data_ready(const char *data, std::size_t size);
    void on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size);
    void on_react_event(const tim::mqtt_topic &topic, const char *data, std::size_t size);
    // Рендер одного сообщения: общая часть on_post и load_post_history.
    void render_post(const tim::uuid &publisher_id, std::string_view text);

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

    tim::signal_connection                      _on_data_ready;
    tim::signal_connection                      _on_posted;
    tim::signal_connection                      _on_connected;
    tim::mqtt_subscription                      _sub_post;
    // Подписки на изменения профилей всех пользователей (не только своего):
    // при /setnick или /seticon кто угодно — обновляем локальный кэш, чтобы
    // в сообщениях имя автора всегда было актуальным.
    tim::mqtt_subscription                      _sub_setnick;
    tim::mqtt_subscription                      _sub_seticon;
    tim::mqtt_subscription                      _sub_setmotto;
    tim::mqtt_subscription                      _sub_react_event;
    // Свои события subscribe/unsubscribe — чтобы поддерживать актуальным
    // локальный кэш подписок без дополнительного запроса в БД.
    tim::mqtt_subscription                      _sub_self_subscribe;
    tim::mqtt_subscription                      _sub_self_unsubscribe;
    // Личные уведомления от сервера (например, "ник занят").
    tim::mqtt_subscription                      _sub_notice;

    // Кэш профилей других пользователей. Заполняется лениво из БД и
    // обновляется по событиям user/setnick/+ и user/seticon/+.
    std::unordered_map<tim::uuid, tim::user>    _known_users;

    // UUID пользователей, на которых подписан текущий пользователь.
    // Используется в on_post для пометки сообщений (звёздочка перед ником).
    std::unordered_set<tim::uuid>               _subscriptions;
};

}

}
