#include "tim_mqtt_subscription.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_subscription_p.h"

#include <cassert>


// Public

tim::mqtt_subscription::mqtt_subscription()
    : tim::non_copyable()
    , _d(new tim::p::mqtt_subscription())
{
}

tim::mqtt_subscription::mqtt_subscription(tim::mqtt_client *client, std::size_t id)
    : tim::non_copyable()
    , _d(new tim::p::mqtt_subscription())
{
    assert(client);

    _d->_client = client;
    _d->_id = id;
}

tim::mqtt_subscription::mqtt_subscription(tim::mqtt_subscription &&other) noexcept
    : tim::non_copyable()
    , _d(std::move(other._d))
{
}

tim::mqtt_subscription::~mqtt_subscription()
{
    if (_d)
        unsubscribe();
}

tim::mqtt_subscription &tim::mqtt_subscription::operator=(tim::mqtt_subscription &&other) noexcept
{
    if (this != &other)
    {
        if (_d)
            unsubscribe();
        _d = std::move(other._d);
    }
    return *this;
}

bool tim::mqtt_subscription::active() const
{
    return _d && _d->_client;
}

void tim::mqtt_subscription::unsubscribe()
{
    if (!_d || !_d->_client)
        return;

    _d->_client->unsubscribe(_d->_id);
    _d->_client = nullptr;
}
