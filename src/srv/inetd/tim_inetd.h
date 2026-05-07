#pragma once

#include "tim_service.h"


namespace tim
{

// Абстрактный inetd-подобный сервис: слушает входящие соединения
// и для каждого создаёт сервис заданного типа. Конкретные реализации:
// tcp_inetd (поверх mongoose) и ssh_inetd (поверх libssh).
class inetd : public tim::service
{

protected:

    explicit inetd(const std::string &name);

public:

    virtual ~inetd();
};

}
