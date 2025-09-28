#include "tim_a_inetd_service.h"


// Protected

tim::a_inetd_service::a_inetd_service(const std::string &name, mg_connection *c)
    : tim::service(name)
    , tim::a_io_device(c)
{
}
