#pragma once

#include <cstddef>
#include <memory>


namespace tim
{

class mqtt_client;

namespace p
{

/**
 * Внутреннее состояние tim::mqtt_subscription (PIMPL).
 */
struct mqtt_subscription
{
    /**
     * Weak-ссылка на контрольный блок клиента. Пока блок существует,
     * lock() возвращает указатель на клиента; после разрушения клиента
     * подписка автоматически считается пустой.
     */
    std::weak_ptr<tim::mqtt_client *> _client;
    /** Идентификатор подписки внутри клиента. */
    std::size_t _id = 0;
};

}

}
