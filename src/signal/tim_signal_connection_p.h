#pragma once

#include <cstddef>


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
    /** Сигнал, к которому привязан слот. */
    tim::a_signal *_signal = nullptr;
    /** Идентификатор слота внутри сигнала. */
    std::size_t _connection_id = 0;
};

}

}
