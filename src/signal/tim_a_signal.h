#pragma once

#include <cstddef>


namespace tim
{

/**
 * Абстрактная база сигнала, позволяющая signal_connection вызывать
 * disconnect() без знания о шаблонном типе слотов.
 */
class a_signal
{

public:

    /** Виртуальный деструктор для безопасного полиморфного удаления. */
    virtual ~a_signal() = default;

    /**
     * Отключает слот по идентификатору. Реализуется наследником
     * tim::signal<...>.
     *
     * \param connection_id Идентификатор подключения.
     * \return true, если слот существовал и был удалён.
     */
    virtual bool disconnect(std::size_t connection_id) = 0;
};

}
