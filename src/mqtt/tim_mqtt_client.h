#pragma once

#include "tim_signal.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>


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

    mqtt_client(mg_mgr *mg, const std::filesystem::path &url = "mqtts://127.0.0.1:8883",
                const std::chrono::seconds ping_interval = std::chrono::seconds{5});
    ~mqtt_client();

    void publish(const std::string &topic,
                 const char *data, std::size_t size,
                 std::uint8_t qos = 1,
                 bool retain = false);

    using message_handler = std::function<void (const std::string &topic, const char *data, std::size_t size)>;

    void subscribe(const std::string &topic, message_handler mh, std::uint8_t qos = 1);

private:


    std::unique_ptr<tim::p::mqtt_client> _d;
};

}
