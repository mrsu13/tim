#pragma once

#include "tim_non_copyable.h"

#include <cstdint>
#include <string>

#include "tim_pimpl.h"


namespace tim
{

namespace p
{

struct service;

}

/**
 * Базовый класс именованных серверных сервисов TIM.
 *
 * Несёт лишь уникальный id и имя — конкретное поведение (подписки
 * MQTT, обработка SSH-канала и т.п.) добавляет наследник. Копирование
 * запрещено: сервисы регистрируют MQTT-подписки и иные ресурсы в
 * конструкторе, и дублирование владельца приводит к двойной отписке.
 */
class service : private tim::non_copyable
{

public:

    virtual ~service();

    std::uint64_t id() const;

    const std::string &name() const;

protected:

    explicit service(const std::string &name);

private:

    /** PIMPL: id и имя. */
    tim::pimpl<tim::p::service> _d;
};

}
