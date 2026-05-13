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
 * Внутреннее состояние tim::user_service (PIMPL).
 *
 * Хранит подписки на все семь топиков и реализует обработчики
 * соответствующих событий MQTT.
 */
struct user_service
{
    /**
     * Запоминает ссылки. Подписки выдаёт subscribe().
     *
     * \param mqtt MQTT-клиент.
     * \param db Подключение к БД.
     */
    user_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
        : _mqtt(mqtt), _db(db)
    {
    }

    /** Выдаёт подписки на все обслуживаемые топики. */
    void subscribe();

    /**
     * user/connect: фиксирует факт подключения пользователя — гарантирует
     * наличие строки в user (INSERT OR IGNORE).
     *
     * \param topic Топик user/connect.
     * \param data Payload — UUID пользователя.
     * \param size Размер payload.
     */
    void connect(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /**
     * user/setnick/&lt;uuid&gt;: обновляет ник пользователя. Если ник занят,
     * шлёт notice через session/notice/&lt;uuid&gt;.
     *
     * \param topic Топик с UUID в последнем уровне.
     * \param data Новый ник.
     * \param size Длина ника.
     */
    void setnick(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /**
     * user/seticon/&lt;uuid&gt;: обновляет иконку пользователя.
     *
     * \param topic Топик с UUID.
     * \param data Новая иконка.
     * \param size Длина.
     */
    void seticon(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /**
     * user/setmotto/&lt;uuid&gt;: обновляет девиз пользователя.
     *
     * \param topic Топик с UUID.
     * \param data Новый девиз.
     * \param size Длина.
     */
    void setmotto(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /**
     * user/setpubkey/&lt;uuid&gt;: сохраняет открытый ключ пользователя
     * в транзакции с ensure_user.
     *
     * \param topic Топик с UUID.
     * \param data OpenSSH-форма ключа.
     * \param size Длина.
     */
    void setpubkey(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /**
     * user/subscribe/&lt;subscriber&gt;: создаёт связь subscriber → publisher
     * в таблице subscription. Гарантирует обоих пользователей через
     * ensure_user в одной транзакции.
     *
     * \param topic Топик с UUID подписчика.
     * \param data Payload: UUID издателя.
     * \param size Длина.
     */
    void subscribe_to(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /**
     * user/unsubscribe/&lt;subscriber&gt;: удаляет связь subscriber → publisher.
     *
     * \param topic Топик с UUID подписчика.
     * \param data Payload: UUID издателя.
     * \param size Длина.
     */
    void unsubscribe_from(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /** MQTT-клиент сервиса. */
    tim::mqtt_client &_mqtt;
    /** Подключение к БД. */
    tim::sqlite_db &_db;

    // Подписки и сигнал-соединения объявлены ПОСЛЕ полей-зависимостей
    // (_mqtt, _db) намеренно: при разрушении они уничтожаются ПЕРВЫМИ,
    // и их обработчики обращаются к _mqtt/_db. Перестановка проверяется
    // static_assert ниже.

    /** Сигнал mqtt.connected — повторно выдаёт подписки при реконнекте. */
    tim::signal_connection _on_connected;
    /** Подписка на user/connect. */
    tim::mqtt_subscription _sub_connect;
    /** Подписка на user/setnick/+. */
    tim::mqtt_subscription _sub_setnick;
    /** Подписка на user/seticon/+. */
    tim::mqtt_subscription _sub_seticon;
    /** Подписка на user/setmotto/+. */
    tim::mqtt_subscription _sub_setmotto;
    /** Подписка на user/setpubkey/+. */
    tim::mqtt_subscription _sub_setpubkey;
    /** Подписка на user/subscribe/+. */
    tim::mqtt_subscription _sub_subscribe;
    /** Подписка на user/unsubscribe/+. */
    tim::mqtt_subscription _sub_unsubscribe;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
static_assert(offsetof(user_service, _on_connected) > offsetof(user_service, _db),
              "MQTT-подписки и сигнал-соединения должны быть объявлены ПОСЛЕ "
              "полей-зависимостей (_mqtt, _db) — они уничтожаются первыми, и их "
              "обработчики читают эти ссылки.");
#pragma GCC diagnostic pop

}

}
