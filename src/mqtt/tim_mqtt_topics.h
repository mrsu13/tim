#pragma once

#include "tim_mqtt_topic.h"

#include <string_view>


namespace tim
{

class uuid;

}

namespace tim::topics
{

// Топики и фильтры MQTT, по которым общаются сервисы TIM. Единое место,
// чтобы случайная опечатка в одной из частей кода не приводила к молчаливо
// неработающей маршрутизации сообщений.

// --- Фиксированные топики ---

// Публикуется prompt_service при подключении. Payload — UUID пользователя.
constexpr std::string_view USER_CONNECT = "user/connect";


// --- Фильтры подписки (с MQTT-метасимволами) ---

constexpr std::string_view USER_SETNICK_FILTER     = "user/setnick/+";
constexpr std::string_view USER_SETICON_FILTER     = "user/seticon/+";
constexpr std::string_view USER_SETMOTTO_FILTER    = "user/setmotto/+";
constexpr std::string_view USER_SETPUBKEY_FILTER   = "user/setpubkey/+";
constexpr std::string_view USER_SUBSCRIBE_FILTER   = "user/subscribe/+";
constexpr std::string_view USER_UNSUBSCRIBE_FILTER = "user/unsubscribe/+";
constexpr std::string_view POST_FILTER             = "post/+/+";
constexpr std::string_view REACT_FILTER            = "react/+/+";
constexpr std::string_view REACT_EVENT_FILTER      = "react_event/+/+";


// --- Конкретные топики, зависящие от UUID ---
//
// Во всех топиках UUID идёт в формате NoBrackets (abc-...), чтобы '/' не
// вступал в конфликт с разделителем уровней MQTT.

// user/setnick/<uuid> — payload: новый ник пользователя <uuid>.
tim::mqtt_topic user_setnick(const tim::uuid &uid);

// user/seticon/<uuid> — payload: новая иконка пользователя <uuid>.
tim::mqtt_topic user_seticon(const tim::uuid &uid);

// user/setmotto/<uuid> — payload: новый девиз пользователя <uuid>.
tim::mqtt_topic user_setmotto(const tim::uuid &uid);

// user/setpubkey/<uuid> — payload: OpenSSH-форма открытого ключа <uuid>.
tim::mqtt_topic user_setpubkey(const tim::uuid &uid);

// user/subscribe/<subscriber> — payload: UUID издателя, на которого
// <subscriber> подписался.
tim::mqtt_topic user_subscribe(const tim::uuid &subscriber);

// user/unsubscribe/<subscriber> — payload: UUID издателя, от которого
// <subscriber> отписался.
tim::mqtt_topic user_unsubscribe(const tim::uuid &subscriber);

// post/<author>/<post-id> — payload: текст сообщения.
tim::mqtt_topic post(const tim::uuid &author, const tim::uuid &post_id);

// react/<post-id>/<reactor> — запрос реакции. Payload: целочисленный
// вес (0 — снятие).
tim::mqtt_topic react(const tim::uuid &post_id, const tim::uuid &reactor);

// react_event/<post-id>/<reactor> — уведомление об успешно записанной
// реакции. Payload: вес.
tim::mqtt_topic react_event(const tim::uuid &post_id, const tim::uuid &reactor);

// session/notice/<uuid> — сообщение, адресованное конкретному
// пользователю (например, "ник занят"). Payload: текст уведомления;
// клиент рендерит его в чате цветом Warning.
tim::mqtt_topic session_notice(const tim::uuid &uid);

}
