#include "tim_signal_connection.h"

#include "tim_a_signal.h"
#include "tim_signal_connection_p.h"

#include <cassert>


// Public

tim::signal_connection::signal_connection(const std::pair<tim::a_signal *, std::size_t> &s_id)
    : tim::non_copyable()
    , _d(new tim::p::signal_connection())
{
    assert(s_id.first);

    _d->_signal = s_id.first;
    _d->_connection_id = s_id.second;
}

tim::signal_connection::~signal_connection()
{
    disconnect();
}

bool tim::signal_connection::connected() const
{
    return _d->_signal;
}

void tim::signal_connection::disconnect()
{
    if (!_d->_signal)
        return;

    _d->_signal->disconnect(_d->_connection_id);
    _d->_signal = nullptr;
}
