#pragma once

#include <cstddef>
#include <memory>


namespace tim
{

class a_signal;

namespace p
{

/**
 * Внутреннее состояние tim::signal_connection (PIMPL).
 */
struct signal_connection
{
    /**
     * Weak-ссылка на контрольный блок сигнала. Пока блок существует,
     * lock() возвращает указатель на сигнал; после разрушения сигнала
     * подключение автоматически считается пустым.
     */
    std::weak_ptr<tim::a_signal *> _signal;
    /** Идентификатор слота внутри сигнала. */
    std::size_t _connection_id = 0;
};

}

}
