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

    /** Виртуальный деструктор для полиморфного удаления. */
    virtual ~service();

    /** \return Уникальный 64-битный идентификатор экземпляра сервиса. */
    std::uint64_t id() const;

    /** \return Имя сервиса, задаваемое в конструкторе. */
    const std::string &name() const;

protected:

    /**
     * Конструктор только для наследников.
     *
     * \param name Имя сервиса (используется в логах).
     */
    explicit service(const std::string &name);

private:

    /** PIMPL: id и имя. */
    std::unique_ptr<tim::p::service> _d;
};

}
