#include "tim_mqtt_subscription.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_subscription_p.h"

#include <cassert>


// Открытые

/** Пустой токен; подписка отсутствует. */
tim::mqtt_subscription::mqtt_subscription()
    : tim::non_copyable()
    , _d(new tim::p::mqtt_subscription())
{
}

/** Связывает токен с конкретной подпиской (client, id). */
tim::mqtt_subscription::mqtt_subscription(tim::mqtt_client *client, std::size_t id)
    : tim::non_copyable()
    , _d(new tim::p::mqtt_subscription())
{
    assert(client);

    _d->_client = client;
    _d->_id = id;
}

/** Move-конструктор. \a other становится пустым. */
tim::mqtt_subscription::mqtt_subscription(tim::mqtt_subscription &&other) noexcept
    : tim::non_copyable()
    , _d(std::move(other._d))
{
}

/** Деструктор. Если подписка активна — отзывает её. */
tim::mqtt_subscription::~mqtt_subscription()
{
    if (_d)
        unsubscribe();
}

/**
 * Move-присваивание. Перед перемещением активная подписка
 * текущего объекта отзывается.
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

/** \return true, если токен хранит активную подписку. */
bool tim::mqtt_subscription::active() const
{
    return _d && _d->_client;
}

/**
 * Отзывает подписку у клиента и помечает токен пустым. Идемпотентно.
 */
void tim::mqtt_subscription::unsubscribe()
{
    if (!_d || !_d->_client)
        return;

    _d->_client->unsubscribe(_d->_id);
    _d->_client = nullptr;
}
