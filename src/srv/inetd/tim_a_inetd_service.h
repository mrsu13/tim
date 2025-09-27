#pragma once

#include "tim_a_io_device.h"
#include "tim_service.h"


namespace tim
{

class a_inetd_service : public tim::service,
                        public tim::a_io_device
{

protected:

    a_inetd_service(const std::string &name, mg_connection *c);
};

}
