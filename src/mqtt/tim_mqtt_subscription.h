#pragma once

#include "tim_non_copyable.h"

#include <cstddef>
#include <memory>

#include "tim_pimpl.h"


namespace tim
{

class mqtt_client;

namespace p
{

struct mqtt_subscription;

}

/**
 * RAII-объект MQTT-подписки.
 *
 * Возвращается mqtt_client::subscribe(); при разрушении автоматически
 * вызывает mqtt_client::unsubscribe(). Перемещаемый, не копируемый —
 * подписка единственная.
 *
 * Хранит weak-ссылку на контрольный блок клиента, поэтому безопасен
 * и в том случае, когда клиент разрушается раньше объекта подписки:
 * unsubscribe() просто не выполняет действий.
 */
class mqtt_subscription : private tim::non_copyable
{

public:

    mqtt_subscription();

    mqtt_subscription(std::weak_ptr<tim::mqtt_client *> client_alive, std::size_t id);

    mqtt_subscription(mqtt_subscription &&other) noexcept;

    ~mqtt_subscription();

    mqtt_subscription &operator=(mqtt_subscription &&other) noexcept;

    bool active() const;

    void unsubscribe();

private:

    /** PIMPL: weak-ссылка на клиента и id подписки. */
    tim::pimpl<tim::p::mqtt_subscription> _d;
};

}
