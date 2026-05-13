#include "tim_service.h"

#include "tim_service_p.h"

#include <cassert>


// Public

/** Виртуальный деструктор для полиморфного удаления. */
tim::service::~service() = default;

/** \return Уникальный 64-битный идентификатор экземпляра сервиса. */
std::uint64_t tim::service::id() const
{
    return _d->_id;
}

/** \return Имя сервиса, задаваемое в конструкторе. */
const std::string &tim::service::name() const
{
    return _d->_name;
}


// Protected

/**
 * Конструктор только для наследников.
 *
 * \param name Имя сервиса (используется в логах).
 */
tim::service::service(const std::string &name)
    : _d()
{
    assert(!name.empty() && "Service name must not be empty.");

    _d->_id = tim::p::service::next_id();
    _d->_name = name;
}


// Private

/**
 * Возвращает следующий уникальный идентификатор. Статический счётчик
 * монотонно увеличивается на каждый вызов; не thread-safe (TIM однопоточен).
 */
std::uint64_t tim::p::service::next_id()
{
    static std::uint64_t id = 0;
    return ++id;
}
