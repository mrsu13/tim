#pragma once

#include "tim_non_copyable.h"

#include <cstddef>
#include <memory>


namespace tim
{

class mqtt_client;

namespace p
{

struct mqtt_subscription;

}

/**
 * RAII-токен MQTT-подписки.
 *
 * Возвращается mqtt_client::subscribe(); при разрушении автоматически
 * вызывает mqtt_client::unsubscribe(). Перемещаемый, не копируемый —
 * подписка единственная.
 */
class mqtt_subscription : private tim::non_copyable
{

public:

    mqtt_subscription();

    mqtt_subscription(tim::mqtt_client *client, std::size_t id);

    mqtt_subscription(mqtt_subscription &&other) noexcept;

    ~mqtt_subscription();

    mqtt_subscription &operator=(mqtt_subscription &&other) noexcept;

    bool active() const;

    void unsubscribe();

private:

    /** PIMPL: указатель на клиента и id подписки. */
    std::unique_ptr<tim::p::mqtt_subscription> _d;
};

}
