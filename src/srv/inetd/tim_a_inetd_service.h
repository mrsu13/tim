#pragma once

#include "tim_a_io_device.h"
#include "tim_service.h"


namespace tim
{

// Базовый класс сервиса, обслуживающего одно соединение в рамках inetd.
// Транспорт реализуется наследниками: a_tcp_inetd_service для mongoose,
// a_ssh_inetd_service для libssh. Этот класс остаётся абстрактным —
// методы a_io_device должны быть переопределены наследником.
class a_inetd_service : public tim::service,
                        public tim::a_io_device
{

protected:

    explicit a_inetd_service(const std::string &name);
};

}
