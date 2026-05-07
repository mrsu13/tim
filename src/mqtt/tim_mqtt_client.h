#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_signal.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
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

    mqtt_client(mg_mgr *mg, std::string_view url = "mqtts://127.0.0.1:8883",
                const std::chrono::seconds ping_interval = std::chrono::seconds{5});
    ~mqtt_client();

    bool is_connected() const;

    void publish(std::string_view topic,
                 const char *data, std::size_t size,
                 std::uint8_t qos = 1,
                 bool retain = false);

    void publish(std::string_view topic,
                 std::string_view payload,
                 std::uint8_t qos = 1,
                 bool retain = false);

    using message_handler = std::function<void (const std::string &topic, const char *data, std::size_t size)>;

    [[nodiscard]] tim::mqtt_subscription subscribe(std::string_view topic_filter,
                                                   message_handler mh,
                                                   std::uint8_t qos = 1);
    void unsubscribe(std::size_t id);

private:


    std::unique_ptr<tim::p::mqtt_client> _d;
};

}
