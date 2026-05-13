#pragma once

#include <cstdint>
#include <memory>
#include <string>


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
 * MQTT, обработка SSH-канала и т.п.) добавляет наследник.
 */
class service
{

public:

    virtual ~service();

    std::uint64_t id() const;

    const std::string &name() const;

protected:

    explicit service(const std::string &name);

private:

    /** PIMPL: id и имя. */
    std::unique_ptr<tim::p::service> _d;
};

}
