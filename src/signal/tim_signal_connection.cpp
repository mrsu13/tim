#include "tim_signal_connection.h"

#include "tim_a_signal.h"
#include "tim_signal_connection_p.h"

#include <cassert>


// Открытые

/** Пустой токен: не связан ни с одним сигналом. */
tim::signal_connection::signal_connection()
    : tim::non_copyable()
    , _d()
{
}

/**
 * Конструирует токен, связанный с конкретным (signal, id).
 *
 * \param s_id Пара (указатель на сигнал, идентификатор слота).
 */
tim::signal_connection::signal_connection(const std::pair<tim::a_signal *, std::size_t> &s_id)
    : tim::non_copyable()
    , _d()
{
    assert(s_id.first);

    _d->_signal = s_id.first;
    _d->_connection_id = s_id.second;
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

/** \return true, если токен хранит активное подключение. */
bool tim::signal_connection::connected() const
{
    return _d && _d->_signal;
}

/**
 * Явно отзывает подключение. Идемпотентно.
 */
void tim::signal_connection::disconnect()
{
    if (!_d || !_d->_signal)
        return;

    _d->_signal->disconnect(_d->_connection_id);
    _d->_signal = nullptr;
}
