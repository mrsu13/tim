#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_mqtt_topic.h"
#include "tim_signal.h"

#include <chrono>
#include <cstdint>
#include <string_view>

#include "tim_pimpl.h"


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
 *
 * Подписки регистрируются на стороне клиента и автоматически повторно
 * оформляются у брокера при каждом восстановлении соединения — сервисам
 * достаточно подписаться один раз. Публикации с QoS > 0 хранятся
 * в очереди до подтверждения PUBACK и повторяются после восстановления
 * соединения (см. tim::MQTT_OUTBOX_LIMIT).
 */
class mqtt_client
{

public:

    /** Испускается при установлении (или восстановлении) соединения с брокером. */
    tim::signal<> connected;
    /** Испускается при потере соединения с брокером (зарезервировано на будущее). */
    tim::signal<> disconnected;

    explicit mqtt_client(mg_mgr *mg);

    ~mqtt_client();

    [[nodiscard]] bool start(std::string_view url = "mqtts://127.0.0.1:8883",
                             const std::chrono::seconds ping_interval = std::chrono::seconds{5});

    void stop();

    bool is_connected() const;

    bool publish(const tim::mqtt_topic &topic,
                 const char *data, std::size_t size,
                 std::uint8_t qos = 1,
                 bool retain = false);

    bool publish(const tim::mqtt_topic &topic,
                 std::string_view payload,
                 std::uint8_t qos = 1,
                 bool retain = false);

    /** Тип обработчика входящего сообщения подписки. */
    using message_handler = std::function<void (const tim::mqtt_topic &topic, const char *data, std::size_t size)>;

    [[nodiscard]] tim::mqtt_subscription subscribe(const tim::mqtt_topic &topic_filter,
                                                   message_handler mh,
                                                   std::uint8_t qos = 1);

    void unsubscribe(std::size_t id);

private:

    /** PIMPL: соединение, таймер, карта подписчиков, очередь публикаций. */
    tim::pimpl<tim::p::mqtt_client> _d;
};

}
