#include "tim_a_inetd_service.h"


// Защищённые

/**
 * Конструктор только для наследников.
 *
 * \param name Имя сервиса (используется в логах).
 */
tim::a_inetd_service::a_inetd_service(const std::string &name)
    : tim::service(name)
    , tim::a_io_device()
{
}
