#include "tim_signal_connection.h"

#include "tim_a_signal.h"
#include "tim_signal_connection_p.h"

#include <cassert>


// Открытые

/** Пустой токен: не связан ни с одним сигналом. */
tim::signal_connection::signal_connection()
    : tim::non_copyable()
    , _d(new tim::p::signal_connection())
{
}

/**
 * Конструирует токен, связанный с (signal, id).
 */
tim::signal_connection::signal_connection(const std::pair<tim::a_signal *, std::size_t> &s_id)
    : tim::non_copyable()
    , _d(new tim::p::signal_connection())
{
    assert(s_id.first);

    _d->_signal = s_id.first;
    _d->_connection_id = s_id.second;
}

/** Move-конструктор. \a other становится пустым. */
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
 * Move-присваивание. Перед перемещением активное подключение
 * текущего объекта отзывается.
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
 * Отзывает подключение и помечает токен пустым. Идемпотентно.
 */
void tim::signal_connection::disconnect()
{
    if (!_d || !_d->_signal)
        return;

    _d->_signal->disconnect(_d->_connection_id);
    _d->_signal = nullptr;
}
