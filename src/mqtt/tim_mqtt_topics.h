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

/**
 * Строит топик "user/setnick/<uuid>".
 *
 * \param uid UUID пользователя, у которого меняется ник.
 * \return Сконструированный топик.
 */
tim::mqtt_topic user_setnick(const tim::uuid &uid);

/**
 * Строит топик "user/seticon/<uuid>".
 *
 * \param uid UUID пользователя.
 * \return Сконструированный топик.
 */
tim::mqtt_topic user_seticon(const tim::uuid &uid);

/**
 * Строит топик "user/setmotto/<uuid>".
 *
 * \param uid UUID пользователя.
 * \return Сконструированный топик.
 */
tim::mqtt_topic user_setmotto(const tim::uuid &uid);

/**
 * Строит топик "user/setpubkey/<uuid>".
 *
 * \param uid UUID пользователя.
 * \return Сконструированный топик.
 */
tim::mqtt_topic user_setpubkey(const tim::uuid &uid);

/**
 * Строит топик "user/subscribe/<subscriber>".
 *
 * \param subscriber UUID подписавшегося пользователя.
 * \return Сконструированный топик; payload — UUID издателя.
 */
tim::mqtt_topic user_subscribe(const tim::uuid &subscriber);

/**
 * Строит топик "user/unsubscribe/<subscriber>".
 *
 * \param subscriber UUID отписавшегося пользователя.
 * \return Сконструированный топик; payload — UUID издателя.
 */
tim::mqtt_topic user_unsubscribe(const tim::uuid &subscriber);

/**
 * Строит топик "post/<author>/<post-id>".
 *
 * \param author UUID автора сообщения.
 * \param post_id UUID самого сообщения.
 * \return Сконструированный топик; payload — текст сообщения.
 */
tim::mqtt_topic post(const tim::uuid &author, const tim::uuid &post_id);

/**
 * Строит топик "react/<post-id>/<reactor>" — запрос реакции.
 *
 * \param post_id UUID сообщения.
 * \param reactor UUID реагирующего пользователя.
 * \return Сконструированный топик; payload — целочисленный вес (0 — снятие).
 */
tim::mqtt_topic react(const tim::uuid &post_id, const tim::uuid &reactor);

/**
 * Строит топик "react_event/<post-id>/<reactor>" — уведомление об успешно
 * записанной реакции.
 *
 * \param post_id UUID сообщения.
 * \param reactor UUID реагирующего пользователя.
 * \return Сконструированный топик; payload — вес реакции.
 */
tim::mqtt_topic react_event(const tim::uuid &post_id, const tim::uuid &reactor);

/**
 * Строит топик "session/notice/<uuid>" — личное уведомление пользователю
 * (например, "ник занят"). Payload — текст; клиент отрисовывает цветом warning.
 *
 * \param uid UUID получателя.
 * \return Сконструированный топик.
 */
tim::mqtt_topic session_notice(const tim::uuid &uid);

}
