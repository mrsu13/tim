#include "tim_a_inetd_service.h"


// Защищённые

tim::a_inetd_service::a_inetd_service(const std::string &name)
    : tim::service(name)
    , tim::a_io_device()
{
}
