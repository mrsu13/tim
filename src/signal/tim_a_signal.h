#pragma once

#include <cstddef>
#include <memory>


namespace tim
{

/**
 * Абстрактная база сигнала, позволяющая signal_connection вызывать
 * disconnect() без знания о шаблонном типе слотов.
 *
 * База владеет контрольным блоком времени жизни (alive()): каждый
 * signal_connection хранит weak_ptr на блок и перед disconnect()
 * проверяет, существует ли ещё сигнал. Благодаря этому объект
 * подключения, переживший свой сигнал, безопасно становится пустым
 * вместо обращения к разрушенной памяти.
 *
 * Сигнал не копируется и не перемещается: контрольный блок хранит
 * указатель this, который должен оставаться действительным всё время
 * жизни сигнала.
 */
class a_signal
{

public:

    /** Создаёт сигнал и его контрольный блок времени жизни. */
    a_signal()
        : _alive(std::make_shared<a_signal *>(this))
    {
    }

    a_signal(const a_signal &) = delete;
    a_signal &operator=(const a_signal &) = delete;

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

    /**
     * \return weak-ссылка на контрольный блок; истекает вместе
     *         с сигналом. Используется signal_connection.
     */
    std::weak_ptr<a_signal *> alive() const
    {
        return _alive;
    }

private:

    /** Контрольный блок: разделяемый указатель на этот сигнал. */
    std::shared_ptr<a_signal *> _alive;
};

}
