#pragma once

#include "tim_a_signal.h"
#include "tim_signal_connection.h"
#include "tim_slot.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>


namespace tim
{

template<typename... Args>
class signal : public tim::a_signal
{

public:

    signal();

    template<typename R>
    [[nodiscard]] tim::signal_connection connect(std::function<R (Args...)> fn);

    [[nodiscard]] tim::signal_connection connect(std::function<void (Args...)> fn);

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
tim::signal_connection tim::signal<Args...>::connect(std::function<R (Args...)> fn)
{
    const std::size_t id = next_id();
    _slots[id] = std::move(std::make_unique<tim::slot<R, Args...>>(fn));
    return tim::signal_connection(std::pair<tim::a_signal *, std::size_t>{ this, id });
}

template<typename... Args>
tim::signal_connection tim::signal<Args...>::connect(std::function<void (Args...)> fn)
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
    // Snapshot connection ids so a slot may safely (dis)connect during emission.
    std::vector<std::size_t> ids;
    ids.reserve(_slots.size());
    for (const typename slot_map::value_type &pair: _slots)
        ids.push_back(pair.first);

    for (std::size_t id: ids)
    {
        const typename slot_map::const_iterator it = _slots.find(id);
        if (it != _slots.cend())
            it->second->invoke(args...);
    }
}


// Private

template<typename... Args>
std::size_t tim::signal<Args...>::next_id()
{
    static std::size_t id = 0;
    return id++;
}
