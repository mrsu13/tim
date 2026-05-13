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
 * Внутреннее состояние tim::reaction_service (PIMPL).
 */
struct reaction_service
{
    /**
     * Запоминает ссылки. Подписку выдаёт subscribe().
     *
     * \param mqtt MQTT-клиент.
     * \param db Подключение к БД.
     */
    reaction_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
        : _mqtt(mqtt), _db(db)
    {
    }

    /** Выдаёт подписку на REACT_FILTER. */
    void subscribe();

    /**
     * Обработчик react/&lt;post&gt;/&lt;reactor&gt;. Парсит UUID-ы и weight,
     * пишет в таблицу reaction в транзакции с ensure_user, затем
     * публикует react_event/&lt;post&gt;/&lt;reactor&gt;.
     *
     * \param topic Топик с UUID-ами.
     * \param data Payload: целочисленный вес.
     * \param size Длина.
     */
    void on_react(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /** MQTT-клиент сервиса. */
    tim::mqtt_client &_mqtt;
    /** Подключение к БД. */
    tim::sqlite_db &_db;

    // Подписки и сигнал-соединения объявлены ПОСЛЕ полей-зависимостей
    // (_mqtt, _db) намеренно: при разрушении они уничтожаются ПЕРВЫМИ,
    // и их обработчики обращаются к _mqtt/_db. Перестановка проверяется
    // static_assert ниже.

    /** Сигнал mqtt.connected — повторно выдаёт подписку. */
    tim::signal_connection _on_connected;
    /** Подписка на REACT_FILTER. */
    tim::mqtt_subscription _sub_react;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
static_assert(offsetof(reaction_service, _on_connected) > offsetof(reaction_service, _db),
              "MQTT-подписки и сигнал-соединения должны быть объявлены ПОСЛЕ "
              "полей-зависимостей (_mqtt, _db) — они уничтожаются первыми, и их "
              "обработчики читают эти ссылки.");
#pragma GCC diagnostic pop

}

}
