#pragma once

#include "tim_mqtt_topic.h"

#include <string_view>


namespace tim
{

class uuid;

}

/**
 * Топики и фильтры MQTT, по которым общаются сервисы TIM. Единое место,
 * чтобы случайная опечатка в одной из частей кода не приводила к молчаливо
 * неработающей маршрутизации сообщений.
 */
namespace tim::topics
{

// --- Фиксированные топики ---

/** Публикуется prompt_service при подключении. Payload — UUID пользователя. */
constexpr std::string_view USER_CONNECT = "user/connect";


// --- Фильтры подписки (с MQTT-метасимволами) ---

/** Фильтр: события смены ника любого пользователя. */
constexpr std::string_view USER_SETNICK_FILTER     = "user/setnick/+";
/** Фильтр: события смены иконки. */
constexpr std::string_view USER_SETICON_FILTER     = "user/seticon/+";
/** Фильтр: события смены девиза. */
constexpr std::string_view USER_SETMOTTO_FILTER    = "user/setmotto/+";
/** Фильтр: публикация открытого ключа пользователя. */
constexpr std::string_view USER_SETPUBKEY_FILTER   = "user/setpubkey/+";
/** Фильтр: события подписки одного пользователя на другого. */
constexpr std::string_view USER_SUBSCRIBE_FILTER   = "user/subscribe/+";
/** Фильтр: события отписки. */
constexpr std::string_view USER_UNSUBSCRIBE_FILTER = "user/unsubscribe/+";
/** Фильтр: входящие сообщения от любого автора. */
constexpr std::string_view POST_FILTER             = "post/+/+";
/** Фильтр: запросы реакций. */
constexpr std::string_view REACT_FILTER            = "react/+/+";
/** Фильтр: уведомления об успешно записанных реакциях. */
constexpr std::string_view REACT_EVENT_FILTER      = "react_event/+/+";


// --- Конкретные топики, зависящие от UUID ---
//
// Во всех топиках UUID идёт в формате no_brackets (abc-...), чтобы '/' не
// вступал в конфликт с разделителем уровней MQTT.

tim::mqtt_topic user_setnick(const tim::uuid &uid);

tim::mqtt_topic user_seticon(const tim::uuid &uid);

tim::mqtt_topic user_setmotto(const tim::uuid &uid);

tim::mqtt_topic user_setpubkey(const tim::uuid &uid);

tim::mqtt_topic user_subscribe(const tim::uuid &subscriber);

tim::mqtt_topic user_unsubscribe(const tim::uuid &subscriber);

tim::mqtt_topic post(const tim::uuid &author, const tim::uuid &post_id);

tim::mqtt_topic react(const tim::uuid &post_id, const tim::uuid &reactor);

tim::mqtt_topic react_event(const tim::uuid &post_id, const tim::uuid &reactor);

tim::mqtt_topic session_notice(const tim::uuid &uid);

}
