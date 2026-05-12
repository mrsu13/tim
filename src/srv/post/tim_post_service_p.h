#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_signal_connection.h"

#include <cstddef>


namespace tim
{

class mqtt_client;
class mqtt_topic;
class sqlite_db;

namespace p
{

/**
 * Внутреннее состояние tim::post_service (PIMPL).
 */
struct post_service
{
    /**
     * Запоминает ссылки на mqtt и db. Подписку выдаст subscribe().
     *
     * \param mqtt MQTT-клиент.
     * \param db Подключение к БД.
     */
    post_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
        : _mqtt(mqtt), _db(db)
    {
    }

    /**
     * Выдаёт подписку на POST_FILTER. Вызывается при первом подключении
     * и на каждом реконнекте.
     */
    void subscribe();

    /**
     * Обработчик входящего поста. Парсит topic ("post/<author>/<id>"),
     * гарантирует автора в user и вставляет запись в post.
     *
     * \param topic Полный топик публикации.
     * \param data Указатель на полезную нагрузку (текст сообщения).
     * \param size Размер нагрузки.
     */
    void on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /** MQTT-клиент сервиса. */
    tim::mqtt_client &_mqtt;
    /** Подключение к БД. */
    tim::sqlite_db &_db;

    /** Подключение к сигналу mqtt.connected для повторной выдачи подписки. */
    tim::signal_connection _on_connected;
    /** Подписка на POST_FILTER. */
    tim::mqtt_subscription _sub_post;
};
}

}
