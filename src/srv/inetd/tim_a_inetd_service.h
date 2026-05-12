#pragma once

#include "tim_a_io_device.h"
#include "tim_service.h"


namespace tim
{

/**
 * Базовый класс сервиса, обслуживающего одно соединение в рамках inetd.
 *
 * Транспорт реализуется наследником a_ssh_inetd_service (поверх libssh);
 * этот класс остаётся абстрактным — методы a_io_device должны быть
 * переопределены наследником.
 */
class a_inetd_service : public tim::service,
                        public tim::a_io_device
{

public:

    /**
     * Сообщение сервису "сворачивайся, мы тебя закрываем".
     *
     * Реализация по умолчанию — no-op; сервисы, которые могут оказаться
     * внутри долгой синхронной работы (например, prompt_service внутри
     * Tcl-скрипта), переопределяют, чтобы прервать её.
     */
    virtual void interrupt() noexcept {}

protected:

    /**
     * Конструктор только для наследников.
     *
     * \param name Имя сервиса (используется в логах).
     */
    explicit a_inetd_service(const std::string &name);
};

}
