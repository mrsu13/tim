#include "tim_signal_connection.h"

#include "tim_a_signal.h"
#include "tim_signal_connection_p.h"

#include <cassert>


// Открытые

/** Пустой объект: не связан ни с одним сигналом. */
tim::signal_connection::signal_connection()
    : tim::non_copyable()
    , _d()
{
}

/**
 * Конструирует объект, связанный с конкретным подключением.
 *
 * \param signal_alive Weak-ссылка на контрольный блок сигнала
 *                     (a_signal::alive()).
 * \param id Идентификатор слота внутри сигнала.
 */
tim::signal_connection::signal_connection(std::weak_ptr<tim::a_signal *> signal_alive,
                                          std::size_t id)
    : tim::non_copyable()
    , _d()
{
    assert(!signal_alive.expired());

    _d->_signal = std::move(signal_alive);
    _d->_connection_id = id;
}

/**
 * Move-конструктор. \a other становится пустым.
 */
tim::signal_connection::signal_connection(tim::signal_connection &&other) noexcept
    : tim::non_copyable()
    , _d(std::move(other._d))
{
}

/** Деструктор. Если подключение активно — отключает слот. */
tim::signal_connection::~signal_connection()
{
    if (_d)
        disconnect();
}

/**
 * Move-присваивание. Если *this хранит активное подключение, оно
 * отзывается; затем перемещается состояние из \a other.
 *
 * \param other Источник перемещения.
 * \return *this.
 */
tim::signal_connection &tim::signal_connection::operator=(tim::signal_connection &&other) noexcept
{
    if (this != &other)
    {
        if (_d)
            disconnect();
        _d = std::move(other._d);
    }
    return *this;
}

/**
 * \return true, если объект хранит активное подключение и сигнал
 *         ещё существует.
 */
bool tim::signal_connection::connected() const
{
    return _d && !_d->_signal.expired();
}

/**
 * Явно отзывает подключение. Идемпотентно; если сигнал уже разрушен,
 * не выполняет действий (weak-ссылка истекла).
 */
void tim::signal_connection::disconnect()
{
    if (!_d)
        return;

    if (const std::shared_ptr<tim::a_signal *> signal = _d->_signal.lock())
        (*signal)->disconnect(_d->_connection_id);
    _d->_signal.reset();
}
