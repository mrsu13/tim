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

    /** Конструирует "пустой" токен — подписка отсутствует. */
    mqtt_subscription();

    /**
     * Конструирует токен, связанный с конкретной подпиской.
     *
     * \param client MQTT-клиент, выдавший подписку.
     * \param id Идентификатор подписки внутри client.
     */
    mqtt_subscription(tim::mqtt_client *client, std::size_t id);

    /**
     * Перемещает подписку из \a other в *this. \a other становится пустым.
     */
    mqtt_subscription(mqtt_subscription &&other) noexcept;

    /** Деструктор. Если подписка активна — вызывает unsubscribe(). */
    ~mqtt_subscription();

    /**
     * Move-присваивание. Если *this хранит активную подписку, она
     * сначала отзывается; затем перемещается состояние из \a other.
     *
     * \param other Источник перемещения.
     * \return *this.
     */
    mqtt_subscription &operator=(mqtt_subscription &&other) noexcept;

    /** \return true, если объект хранит активную подписку. */
    bool active() const;

    /**
     * Явно отзывает подписку. Идемпотентно: повторный вызов на пустом
     * или уже отозванном токене безопасен.
     */
    void unsubscribe();

private:

    /** PIMPL: указатель на клиента и id подписки. */
    std::unique_ptr<tim::p::mqtt_subscription> _d;
};

}
