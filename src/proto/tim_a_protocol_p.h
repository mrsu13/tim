#pragma once

#include "tim_signal_connection.h"

#include <cassert>


namespace tim
{

class a_io_device;
class a_protocol;

namespace p
{

/**
 * Внутреннее состояние tim::a_protocol (PIMPL).
 */
struct a_protocol
{
    /**
     * \param q Указатель на внешний a_protocol (для делегирования
     *          process_raw_data() из сигнал-обработчика).
     */
    explicit a_protocol(tim::a_protocol *q)
        : _q(q)
    {
        assert(_q);
    }

    /**
     * Обработчик сигнала io->ready_read. Считывает байты из транспорта
     * и передаёт в _q->process_raw_data().
     */
    void on_ready_read();

    /** Внешний протокол (владелец). */
    tim::a_protocol *const _q;
    /** Указатель на транспорт. */
    tim::a_io_device *_io = nullptr;
    /** Подключение к сигналу ready_read транспорта. */
    tim::signal_connection _on_ready_read;
};

}

}
