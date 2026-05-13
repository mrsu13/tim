#pragma once

#include "tim_mqtt_client.h"

#include <cassert>
#include <string>
#include <vector>


struct mg_connection;
struct mg_timer;

namespace tim::p
{

/**
 * Внутреннее состояние tim::mqtt_client (PIMPL).
 *
 * Хранит mongoose-соединение и таймер keep-alive, URL брокера, список
 * активных подписчиков. Статические методы — обработчики событий
 * mongoose (MG_EV_*).
 */
struct mqtt_client
{
    /**
     * \param q Указатель на внешний mqtt_client (для делегирования
     *          событий).
     */
    explicit mqtt_client(tim::mqtt_client *q)
        : _q(q)
    {
        assert(_q);
    }

    /**
     * Mongoose-обработчик событий соединения.
     *
     * \param c Активное mongoose-соединение.
     * \param ev Тип события (MG_EV_CONNECT, MG_EV_MQTT_MSG и т.п.).
     * \param ev_data Контекст события (зависит от ev).
     */
    static void handle_events(mg_connection *c, int ev, void *ev_data);

    /**
     * Mongoose-обработчик таймера keep-alive: переподключается при
     * отсутствии соединения, шлёт PING при наличии.
     *
     * \param data Указатель на p::mqtt_client.
     */
    static void ping(void *data);

    /** Внешний mqtt_client (владелец). */
    tim::mqtt_client *const _q;

    /** Mongoose-менеджер (не владеем). */
    mg_mgr *_mg = nullptr;
    /** URL брокера, заданный в start(). */
    std::string _url;
    /** Активное mongoose-соединение к брокеру; nullptr если отсутствует. */
    mg_connection *_client = nullptr;
    /** Mongoose-таймер для keep-alive/реконнекта. */
    mg_timer *_timer = nullptr;
    /** true после успешного MQTT-handshake. */
    bool _connected = false;

    /**
     * Запись о подписчике: id, фильтр топика, обработчик сообщения.
     */
    struct subscriber_entry
    {
        std::size_t id;                              ///< Идентификатор подписки.
        tim::mqtt_topic filter;                      ///< Фильтр подписки (с + и/или #).
        tim::mqtt_client::message_handler handler;   ///< Обработчик входящих сообщений.
    };
    /** Тип списка подписчиков. */
    using subscribers = std::vector<subscriber_entry>;
    /** Список активных подписчиков. */
    subscribers _subscribers;
    /** Счётчик id для новых подписок. */
    std::size_t _next_subscriber_id = 0;
};

}
