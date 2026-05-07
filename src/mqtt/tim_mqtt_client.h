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

class mqtt_client
{

public:

    tim::signal<> connected;
    tim::signal<> disconnected;

    explicit mqtt_client(mg_mgr *mg);
    ~mqtt_client();

    // Запускает периодический таймер для подключения и keep-alive. Возвращает
    // false при синхронной ошибке (например, не удалось создать таймер).
    // Сетевые ошибки и недоступность брокера не считаются синхронными ошибками —
    // таймер повторит попытку на следующем тике.
    [[nodiscard]] bool start(std::string_view url = "mqtts://127.0.0.1:8883",
                             const std::chrono::seconds ping_interval = std::chrono::seconds{5});

    // Останавливает таймер и закрывает текущее соединение, если оно есть. Идемпотентен.
    void stop();

    bool is_connected() const;

    void publish(const tim::mqtt_topic &topic,
                 const char *data, std::size_t size,
                 std::uint8_t qos = 1,
                 bool retain = false);

    void publish(const tim::mqtt_topic &topic,
                 std::string_view payload,
                 std::uint8_t qos = 1,
                 bool retain = false);

    using message_handler = std::function<void (const tim::mqtt_topic &topic, const char *data, std::size_t size)>;

    [[nodiscard]] tim::mqtt_subscription subscribe(const tim::mqtt_topic &topic_filter,
                                                   message_handler mh,
                                                   std::uint8_t qos = 1);
    void unsubscribe(std::size_t id);

private:


    std::unique_ptr<tim::p::mqtt_client> _d;
};

}
