#pragma once

#include "tim_service.h"


namespace tim
{

/**
 * Абстрактный inetd-подобный сервис: слушает входящие соединения
 * и для каждого создаёт сервис заданного типа. Конкретная реализация —
 * ssh_inetd (поверх libssh).
 */
class inetd : public tim::service
{

protected:

    /**
     * Конструктор только для наследников.
     *
     * \param name Имя сервиса (используется в логах).
     */
    explicit inetd(const std::string &name);

public:

    /** Виртуальный деструктор для полиморфного удаления. */
    virtual ~inetd();
};

}
