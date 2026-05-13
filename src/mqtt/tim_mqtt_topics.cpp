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


/**
 * Строит топик "user/setnick/&lt;uuid&gt;".
 *
 * \param uid UUID пользователя, у которого меняется ник.
 * \return Сконструированный топик.
 */
tim::mqtt_topic tim::topics::user_setnick(const tim::uuid &uid)
{
    return tim::mqtt_topic("user/setnick") / nb(uid);
}

/**
 * Строит топик "user/seticon/&lt;uuid&gt;".
 *
 * \param uid UUID пользователя.
 * \return Сконструированный топик.
 */
tim::mqtt_topic tim::topics::user_seticon(const tim::uuid &uid)
{
    return tim::mqtt_topic("user/seticon") / nb(uid);
}

/**
 * Строит топик "user/setmotto/&lt;uuid&gt;".
 *
 * \param uid UUID пользователя.
 * \return Сконструированный топик.
 */
tim::mqtt_topic tim::topics::user_setmotto(const tim::uuid &uid)
{
    return tim::mqtt_topic("user/setmotto") / nb(uid);
}

/**
 * Строит топик "user/setpubkey/&lt;uuid&gt;".
 *
 * \param uid UUID пользователя.
 * \return Сконструированный топик.
 */
tim::mqtt_topic tim::topics::user_setpubkey(const tim::uuid &uid)
{
    return tim::mqtt_topic("user/setpubkey") / nb(uid);
}

/**
 * Строит топик "user/subscribe/&lt;subscriber&gt;".
 *
 * \param subscriber UUID подписавшегося пользователя.
 * \return Сконструированный топик; payload — UUID издателя.
 */
tim::mqtt_topic tim::topics::user_subscribe(const tim::uuid &subscriber)
{
    return tim::mqtt_topic("user/subscribe") / nb(subscriber);
}

/**
 * Строит топик "user/unsubscribe/&lt;subscriber&gt;".
 *
 * \param subscriber UUID отписавшегося пользователя.
 * \return Сконструированный топик; payload — UUID издателя.
 */
tim::mqtt_topic tim::topics::user_unsubscribe(const tim::uuid &subscriber)
{
    return tim::mqtt_topic("user/unsubscribe") / nb(subscriber);
}

/**
 * Строит топик "post/&lt;author&gt;/&lt;post-id&gt;".
 *
 * \param author UUID автора сообщения.
 * \param post_id UUID самого сообщения.
 * \return Сконструированный топик; payload — текст сообщения.
 */
tim::mqtt_topic tim::topics::post(const tim::uuid &author, const tim::uuid &post_id)
{
    return tim::mqtt_topic("post") / nb(author) / nb(post_id);
}

/**
 * Строит топик "react/&lt;post-id&gt;/&lt;reactor&gt;" — запрос реакции.
 *
 * \param post_id UUID сообщения.
 * \param reactor UUID реагирующего пользователя.
 * \return Сконструированный топик; payload — целочисленный вес (0 — снятие).
 */
tim::mqtt_topic tim::topics::react(const tim::uuid &post_id, const tim::uuid &reactor)
{
    return tim::mqtt_topic("react") / nb(post_id) / nb(reactor);
}

/**
 * Строит топик "react_event/&lt;post-id&gt;/&lt;reactor&gt;" — уведомление об успешно
 * записанной реакции.
 *
 * \param post_id UUID сообщения.
 * \param reactor UUID реагирующего пользователя.
 * \return Сконструированный топик; payload — вес реакции.
 */
tim::mqtt_topic tim::topics::react_event(const tim::uuid &post_id, const tim::uuid &reactor)
{
    return tim::mqtt_topic("react_event") / nb(post_id) / nb(reactor);
}

/**
 * Строит топик "session/notice/&lt;uuid&gt;" — личное уведомление пользователю
 * (например, "ник занят"). Payload — текст; клиент отрисовывает цветом warning.
 *
 * \param uid UUID получателя.
 * \return Сконструированный топик.
 */
tim::mqtt_topic tim::topics::session_notice(const tim::uuid &uid)
{
    return tim::mqtt_topic("session/notice") / nb(uid);
}
