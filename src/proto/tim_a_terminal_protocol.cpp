#include "tim_a_terminal_protocol.h"


// Public

/**
 * Конструктор. Делегирует подписку транспорта в базу a_protocol.
 */
tim::a_terminal_protocol::a_terminal_protocol(tim::a_io_device *io)
    : tim::a_protocol(io)
{
}
