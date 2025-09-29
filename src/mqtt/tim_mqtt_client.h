#pragma once

#include "tim_signal.h"

#include <cstdint>
#include <memory>
#include <string>


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

    mqtt_client(mg_mgr *mg,
                std::uint16_t port = 1883,
                const std::string &host = "localhost");
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
