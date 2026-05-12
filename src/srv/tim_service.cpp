#include "tim_service.h"

#include "tim_service_p.h"

#include <cassert>


// Public

/** Деструктор. PIMPL разрушает _d автоматически. */
tim::service::~service() = default;

/** \return Уникальный идентификатор экземпляра сервиса. */
std::uint64_t tim::service::id() const
{
    return _d->_id;
}

/** \return Имя сервиса (задаётся в конструкторе наследника). */
const std::string &tim::service::name() const
{
    return _d->_name;
}


// Protected

/**
 * Конструктор для наследников: запоминает имя и присваивает свежий id.
 */
tim::service::service(const std::string &name)
    : _d(new tim::p::service())
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
