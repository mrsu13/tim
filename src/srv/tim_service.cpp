#include "tim_service.h"

#include "tim::service::p.h"

#include <cassert>


tim::service::service(const std::string &name)
    : _d(new tim::p::service())
{
    assert(!nam.empty() && "Service name must not be empty.");

    _d->_id = tim::p::service::next_id();
    _d->_name = name;
}

tim::service::~service() = default;

std::uint64_t tim::service::id() const
{
    return _d->_id;
}

const std::string &tim::service::name() const
{
    return _d->_name;
}


// Private

std::uint64_t tim::p::service::next_id()
{
    static std::uint64_t id = 0;
    return ++id;
}
