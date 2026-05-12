#pragma once

#include "tim_a_slot.h"

#include <cassert>
#include <functional>


namespace tim
{

/**
 * Конкретный слот: обёртка над std::function<R(Args...)>.
 *
 * R игнорируется при вызове (operator() не возвращает значение
 * подписчикам сигнала), но участвует в типизации, чтобы можно
 * было подключать функции, возвращающие что-то отличное от void.
 *
 * \tparam R Тип возвращаемого значения функции (игнорируется при вызове).
 * \tparam Args Типы аргументов.
 */
template<typename R, typename... Args>
class slot : public a_slot<Args...>
{

public:

    /**
     * Конструктор.
     *
     * \param fn Функция/лямбда, которую вызовет invoke(). Не должна
     *           быть пустой (проверяется ассертом).
     */
    slot(std::function<R (Args...)> fn)
        : _fn(fn)
    {
        assert(_fn);
    }

    /**
     * Вызывает завёрнутую функцию.
     *
     * \param args Аргументы, прокидываемые в функцию.
     */
    void invoke(Args... args) const override
    {
        _fn(args...);
    }

private:

    /** Хранимая функция. */
    std::function<R (Args...)> _fn;
};

}
