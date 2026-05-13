#pragma once

#include "tim_non_copyable.h"

#include <cstddef>
#include <utility>

#include "tim_pimpl.h"


namespace tim
{

class a_signal;

namespace p
{

struct signal_connection;

}

/**
 * RAII-токен подключения слота к сигналу. При разрушении вызывает
 * disconnect() на сигнале. Перемещаемый, не копируемый.
 */
class signal_connection : private tim::non_copyable
{

public:

    signal_connection();

    signal_connection(const std::pair<tim::a_signal *, std::size_t> &s_id);

    signal_connection(signal_connection &&other) noexcept;

    ~signal_connection();

    signal_connection &operator=(signal_connection &&other) noexcept;

    bool connected() const;

    void disconnect();

private:

    /** PIMPL: указатель на сигнал и id слота. */
    tim::pimpl<tim::p::signal_connection> _d;
};

}
