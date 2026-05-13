#include "tim_mqtt_subscription.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_subscription_p.h"

#include <cassert>


// Открытые

/** Конструирует "пустой" токен — подписка отсутствует. */
tim::mqtt_subscription::mqtt_subscription()
    : tim::non_copyable()
    , _d(new tim::p::mqtt_subscription())
{
}

/**
 * Конструирует токен, связанный с конкретной подпиской (client, id).
 *
 * \param client MQTT-клиент, выдавший подписку.
 * \param id Идентификатор подписки внутри client.
 */
tim::mqtt_subscription::mqtt_subscription(tim::mqtt_client *client, std::size_t id)
    : tim::non_copyable()
    , _d(new tim::p::mqtt_subscription())
{
    assert(client);

    _d->_client = client;
    _d->_id = id;
}

/**
 * Перемещает подписку из \a other в *this. \a other становится пустым.
 */
tim::mqtt_subscription::mqtt_subscription(tim::mqtt_subscription &&other) noexcept
    : tim::non_copyable()
    , _d(std::move(other._d))
{
}

/** Деструктор. Если подписка активна — вызывает unsubscribe(). */
tim::mqtt_subscription::~mqtt_subscription()
{
    if (_d)
        unsubscribe();
}

/**
 * Move-присваивание. Если *this хранит активную подписку, она сначала
 * отзывается; затем перемещается состояние из \a other.
 *
 * \param other Источник перемещения.
 * \return *this.
 */
tim::mqtt_subscription &tim::mqtt_subscription::operator=(tim::mqtt_subscription &&other) noexcept
{
    if (this != &other)
    {
        if (_d)
            unsubscribe();
        _d = std::move(other._d);
    }
    return *this;
}

/** \return true, если объект хранит активную подписку. */
bool tim::mqtt_subscription::active() const
{
    return _d && _d->_client;
}

/**
 * Явно отзывает подписку у клиента и помечает токен пустым.
 * Идемпотентно: повторный вызов на пустом или уже отозванном токене
 * безопасен.
 */
void tim::mqtt_subscription::unsubscribe()
{
    if (!_d || !_d->_client)
        return;

    _d->_client->unsubscribe(_d->_id);
    _d->_client = nullptr;
}
