#pragma once

#include <memory>
#include <utility>


namespace tim
{

/**
 * Псевдоним над std::unique_ptr<T> для PIMPL: переадресует аргументы
 * конструктора прямо в T, избавляя владельца от `new T(args...)`.
 * Семантика идентична std::unique_ptr: не копируется, перемещается,
 * владеет объектом. T можно объявить вперёд.
 *
 * Дисциплина владельца та же, что и для обычного PIMPL: класс-владелец
 * ДОЛЖЕН объявить деструктор out-of-line в .h и определить его в .cpp,
 * где T определён полностью (через `*_p.h`). Иначе в TU без `*_p.h`
 * компилятор попытается синтезировать неявный ~Owner(), которому
 * понадобится полный T.
 *
 * Пример:
 * \code
 * // x.h
 * namespace tim::p { struct x; }
 * class x_owner {
 * public:
 *     x_owner(int a, double b);
 *     ~x_owner();                  // out-of-line, тело в .cpp.
 * private:
 *     tim::pimpl<tim::p::x> _d;
 * };
 *
 * // x.cpp
 * #include "x_p.h"
 * x_owner::x_owner(int a, double b) : _d(a, b) {}
 * x_owner::~x_owner() = default;
 * \endcode
 */
template <typename T>
class pimpl
{

public:

    /**
     * Создаёт T с переданными аргументами.
     *
     * \param args Аргументы конструктора T (perfect-forwarding).
     */
    template <typename... Args>
    explicit pimpl(Args &&...args)
        : _p(new T(std::forward<Args>(args)...))
    {}

    /** Деструктор — освобождает T через unique_ptr. */
    ~pimpl() = default;

    /** Перемещение разрешено. */
    pimpl(pimpl &&) noexcept = default;
    /** Перемещение-присваивание разрешено. */
    pimpl &operator=(pimpl &&) noexcept = default;

    /** Копирование запрещено. */
    pimpl(const pimpl &) = delete;
    /** Копирование-присваивание запрещено. */
    pimpl &operator=(const pimpl &) = delete;

    /** Доступ к членам T. */
    T *operator->() noexcept { return _p.get(); }
    /** Доступ к членам T (const). */
    const T *operator->() const noexcept { return _p.get(); }

    /** Разыменование. */
    T &operator*() noexcept { return *_p; }
    /** Разыменование (const). */
    const T &operator*() const noexcept { return *_p; }

    /** Сырой указатель. */
    T *get() noexcept { return _p.get(); }
    /** Сырой указатель (const). */
    const T *get() const noexcept { return _p.get(); }

    /** \return true, если внутренний указатель ненулевой. */
    explicit operator bool() const noexcept { return (bool)_p; }

private:

    std::unique_ptr<T> _p;
};

}
