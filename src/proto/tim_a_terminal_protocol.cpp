#include "tim_a_terminal_protocol.h"


// Открытые

/**
 * Конструктор.
 *
 * \param io Транспорт; должен жить дольше протокола.
 */
tim::a_terminal_protocol::a_terminal_protocol(tim::a_io_device *io)
    : tim::a_protocol(io)
{
}
