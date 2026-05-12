#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_mqtt_topic.h"
#include "tim_signal.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>


struct mg_mgr;

namespace tim
{

namespace p
{

struct mqtt_client;

}

/**
 * MQTT-клиент на базе libmongoose.
 *
 * Подключается к брокеру по URL, удерживает соединение через периодический
 * keep-alive, маршрутизирует входящие сообщения зарегистрированным
 * подпискам и публикует сигналы connected/disconnected для подсистем,
 * которые должны реагировать на смену состояния.
 */
class mqtt_client
{

public:

    /** Испускается при установлении (или восстановлении) соединения с брокером. */
    tim::signal<> connected;
    /** Испускается при потере соединения с брокером (зарезервировано на будущее). */
    tim::signal<> disconnected;

    /**
     * Конструктор. Не делает сетевых операций; реальное соединение
     * открывается в start().
     *
     * \param mg Mongoose-менеджер, владелец таймеров и соединений.
     *           Должен жить дольше mqtt_client.
     */
    explicit mqtt_client(mg_mgr *mg);

    /** Деструктор; вызывает stop() при необходимости. */
    ~mqtt_client();

    /**
     * Запускает периодический таймер для подключения и keep-alive.
     * Сетевые ошибки и недоступность брокера не считаются синхронными
     * ошибками — таймер повторит попытку на следующем тике.
     *
     * \param url URL брокера: mqtts:// (TLS) или mqtt:// (без TLS).
     * \param ping_interval Период keep-alive PING.
     * \return false при синхронной ошибке (например, не удалось создать
     *         таймер); true при успешной постановке таймера.
     */
    [[nodiscard]] bool start(std::string_view url = "mqtts://127.0.0.1:8883",
                             const std::chrono::seconds ping_interval = std::chrono::seconds{5});

    /**
     * Останавливает таймер и закрывает текущее соединение. Идемпотентен.
     */
    void stop();

    /** \return true, если MQTT-handshake завершился и соединение живо. */
    bool is_connected() const;

    /**
     * Публикует сообщение в топик. Если соединение не установлено,
     * сообщение тихо отбрасывается.
     *
     * \param topic MQTT-топик публикации.
     * \param data Указатель на полезную нагрузку.
     * \param size Размер полезной нагрузки в байтах.
     * \param qos QoS уровень (0/1/2). По умолчанию 1.
     * \param retain Retain-флаг (брокер запомнит последнее сообщение).
     */
    void publish(const tim::mqtt_topic &topic,
                 const char *data, std::size_t size,
                 std::uint8_t qos = 1,
                 bool retain = false);

    /**
     * Удобный перегруз publish() для string_view нагрузки.
     *
     * \param topic MQTT-топик публикации.
     * \param payload Полезная нагрузка.
     * \param qos QoS уровень. По умолчанию 1.
     * \param retain Retain-флаг.
     */
    void publish(const tim::mqtt_topic &topic,
                 std::string_view payload,
                 std::uint8_t qos = 1,
                 bool retain = false);

    /** Тип обработчика входящего сообщения подписки. */
    using message_handler = std::function<void (const tim::mqtt_topic &topic, const char *data, std::size_t size)>;

    /**
     * Подписывается на топик/фильтр у брокера.
     *
     * \param topic_filter Фильтр (поддерживаются + и #).
     * \param mh Обработчик входящих сообщений, удовлетворяющих фильтру.
     * \param qos QoS уровень подписки. По умолчанию 1.
     * \return RAII-токен подписки; при разрушении автоматически отписывается.
     *         [[nodiscard]] — нельзя терять токен молча.
     */
    [[nodiscard]] tim::mqtt_subscription subscribe(const tim::mqtt_topic &topic_filter,
                                                   message_handler mh,
                                                   std::uint8_t qos = 1);

    /**
     * Снимает подписку по идентификатору. Вызывается деструктором
     * mqtt_subscription; редко нужно напрямую.
     *
     * \param id Идентификатор подписки.
     */
    void unsubscribe(std::size_t id);

private:

    /** PIMPL: соединение, таймер, карта подписчиков, очередь публикаций. */
    std::unique_ptr<tim::p::mqtt_client> _d;
};

}
