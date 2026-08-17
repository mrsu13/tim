#pragma once

#include "tim_non_copyable.h"

#include <cstddef>
#include <memory>

#include "tim_pimpl.h"


namespace tim
{

class a_signal;

namespace p
{

struct signal_connection;

}

/**
 * RAII-объект подключения слота к сигналу. При разрушении вызывает
 * disconnect() на сигнале. Перемещаемый, не копируемый.
 *
 * Хранит weak-ссылку на контрольный блок сигнала (a_signal::alive()),
 * поэтому безопасен и в том случае, когда сигнал разрушается раньше
 * объекта подключения: disconnect() просто не выполняет действий.
 */
class signal_connection : private tim::non_copyable
{

public:

    signal_connection();

    signal_connection(std::weak_ptr<tim::a_signal *> signal_alive, std::size_t id);

    signal_connection(signal_connection &&other) noexcept;

    ~signal_connection();

    signal_connection &operator=(signal_connection &&other) noexcept;

    bool connected() const;

    void disconnect();

private:

    /** PIMPL: weak-ссылка на сигнал и id слота. */
    tim::pimpl<tim::p::signal_connection> _d;
};

}
