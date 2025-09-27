#pragma once

#include "tim_a_signal.h"
#include "tim_slot.h"

#include <memory>
#include <unordered_map>
#include <utility>


namespace tim
{

template<typename... Args>
class signal : public tim::a_signal
{

public:

    signal();

    template<typename R>
    std::pair<tim::a_signal *, std::size_t> connect(std::function<R (Args...)> fn);

    std::pair<tim::a_signal *, std::size_t> connect(std::function<void (Args...)> fn);

    bool disconnect(std::size_t connection_id) override;

    void operator()(Args... args) const;

private:

    inline static std::size_t next_id();

    using slot_map = std::unordered_map<std::size_t, std::unique_ptr<tim::a_slot<Args...>>>;
    slot_map _slots;
};

}


// Implementation

// Public

template<typename... Args>
tim::signal<Args...>::signal()
    : tim::a_signal()
    , _slots()
{
}

template<typename... Args>
template<typename R>
std::pair<tim::a_signal *, std::size_t> tim::signal<Args...>::connect(std::function<R (Args...)> fn)
{
    const std::size_t id = next_id();
    _slots[id] = std::move(std::make_unique<tim::slot<R, Args...>>(fn));
    return { this, id };
}

template<typename... Args>
std::pair<tim::a_signal *, std::size_t> tim::signal<Args...>::connect(std::function<void (Args...)> fn)
{
    return connect<void>(fn);
}

template<typename... Args>
bool tim::signal<Args...>::disconnect(std::size_t connection_id)
{
    return _slots.erase(connection_id);
}

template<typename... Args>
void tim::signal<Args...>::operator()(Args... args) const
{
    for (const typename slot_map::value_type &pair: _slots)
        pair.second->invoke(args...);
}


// Private

template<typename... Args>
std::size_t tim::signal<Args...>::next_id()
{
    static std::size_t id = 0;
    return id++;
}
