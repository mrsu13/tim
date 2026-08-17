#include "tim_mqtt_subscription.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_subscription_p.h"

#include <cassert>


// Открытые

/** Конструирует пустой объект — подписка отсутствует. */
tim::mqtt_subscription::mqtt_subscription()
    : tim::non_copyable()
    , _d()
{
}

/**
 * Конструирует объект, связанный с конкретной подпиской.
 *
 * \param client_alive Weak-ссылка на контрольный блок клиента,
 *                     выдавшего подписку.
 * \param id Идентификатор подписки внутри клиента.
 */
tim::mqtt_subscription::mqtt_subscription(std::weak_ptr<tim::mqtt_client *> client_alive,
                                          std::size_t id)
    : tim::non_copyable()
    , _d()
{
    assert(!client_alive.expired());

    _d->_client = std::move(client_alive);
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

/**
 * \return true, если объект хранит активную подписку и клиент
 *         ещё существует.
 */
bool tim::mqtt_subscription::active() const
{
    return _d && !_d->_client.expired();
}

/**
 * Явно отзывает подписку у клиента и помечает объект пустым.
 * Идемпотентно: повторный вызов на пустом или уже отозванном объекте
 * безопасен; если клиент уже разрушен, не выполняет действий
 * (weak-ссылка истекла).
 */
void tim::mqtt_subscription::unsubscribe()
{
    if (!_d)
        return;

    if (const std::shared_ptr<tim::mqtt_client *> client = _d->_client.lock())
        (*client)->unsubscribe(_d->_id);
    _d->_client.reset();
}
