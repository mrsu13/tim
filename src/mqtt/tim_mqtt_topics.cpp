#include "tim_mqtt_topics.h"

#include "tim_uuid.h"


namespace
{

/**
 * Возвращает строковое представление UUID в формате no_brackets —
 * без фигурных скобок. Чтобы '/' внутри топика не вступал в конфликт
 * с разделителем уровней MQTT.
 *
 * \param uid UUID.
 * \return Строка вида "943b573e-7a1d-4419-81b1-3308455be5f7".
 */
inline std::string nb(const tim::uuid &uid)
{
    return uid.to_string(tim::uuid::format::no_brackets);
}

}


tim::mqtt_topic tim::topics::user_setnick(const tim::uuid &uid)
{
    return tim::mqtt_topic("user/setnick") / nb(uid);
}

tim::mqtt_topic tim::topics::user_seticon(const tim::uuid &uid)
{
    return tim::mqtt_topic("user/seticon") / nb(uid);
}

tim::mqtt_topic tim::topics::user_setmotto(const tim::uuid &uid)
{
    return tim::mqtt_topic("user/setmotto") / nb(uid);
}

tim::mqtt_topic tim::topics::user_setpubkey(const tim::uuid &uid)
{
    return tim::mqtt_topic("user/setpubkey") / nb(uid);
}

tim::mqtt_topic tim::topics::user_subscribe(const tim::uuid &subscriber)
{
    return tim::mqtt_topic("user/subscribe") / nb(subscriber);
}

tim::mqtt_topic tim::topics::user_unsubscribe(const tim::uuid &subscriber)
{
    return tim::mqtt_topic("user/unsubscribe") / nb(subscriber);
}

tim::mqtt_topic tim::topics::post(const tim::uuid &author, const tim::uuid &post_id)
{
    return tim::mqtt_topic("post") / nb(author) / nb(post_id);
}

tim::mqtt_topic tim::topics::react(const tim::uuid &post_id, const tim::uuid &reactor)
{
    return tim::mqtt_topic("react") / nb(post_id) / nb(reactor);
}

tim::mqtt_topic tim::topics::react_event(const tim::uuid &post_id, const tim::uuid &reactor)
{
    return tim::mqtt_topic("react_event") / nb(post_id) / nb(reactor);
}

tim::mqtt_topic tim::topics::session_notice(const tim::uuid &uid)
{
    return tim::mqtt_topic("session/notice") / nb(uid);
}
