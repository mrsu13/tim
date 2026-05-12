#pragma once

#include "tim_non_copyable.h"

#include <cstddef>
#include <memory>
#include <utility>


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

    /** Пустой токен: не связан ни с одним сигналом. */
    signal_connection();

    /**
     * Конструирует токен, связанный с конкретным (signal, id).
     *
     * \param s_id Пара (указатель на сигнал, идентификатор слота).
     */
    signal_connection(const std::pair<tim::a_signal *, std::size_t> &s_id);

    /**
     * Move-конструктор. \a other становится пустым.
     */
    signal_connection(signal_connection &&other) noexcept;

    /** Деструктор. Если подключение активно — отключает слот. */
    ~signal_connection();

    /**
     * Move-присваивание. Если *this хранит активное подключение, оно
     * отзывается; затем перемещается состояние из \a other.
     *
     * \param other Источник перемещения.
     * \return *this.
     */
    signal_connection &operator=(signal_connection &&other) noexcept;

    /** \return true, если токен хранит активное подключение. */
    bool connected() const;

    /**
     * Явно отзывает подключение. Идемпотентно.
     */
    void disconnect();

private:

    /** PIMPL: указатель на сигнал и id слота. */
    std::unique_ptr<tim::p::signal_connection> _d;
};

}
