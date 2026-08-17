#pragma once

#include "tim_mqtt_subscription.h"

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
     * Оформляет подписку на POST_FILTER. Вызывается при первом подключении
     * и при каждом восстановлении соединения.
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

    // Подписки и сигнал-соединения объявлены ПОСЛЕ полей-зависимостей
    // (_mqtt, _db) намеренно: при разрушении они уничтожаются ПЕРВЫМИ,
    // и их обработчики обращаются к _mqtt/_db. Если поменять порядок,
    // обработчик увидит уже разрушенную ссылку. Перестановка проверяется
    // static_assert ниже.

    /** Подписка на POST_FILTER. */
    tim::mqtt_subscription _sub_post;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
static_assert(offsetof(post_service, _sub_post) > offsetof(post_service, _db),
              "MQTT-подписки и сигнал-соединения должны быть объявлены ПОСЛЕ "
              "полей-зависимостей (_mqtt, _db) — они уничтожаются первыми, и их "
              "обработчики читают эти ссылки.");
#pragma GCC diagnostic pop

}

}
