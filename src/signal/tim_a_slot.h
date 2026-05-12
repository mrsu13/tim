#pragma once

namespace tim
{

/**
 * Абстрактный слот сигнала: вызывается с типизированным списком
 * аргументов \a Args.
 *
 * tim::signal<Args...> хранит unique_ptr<a_slot<Args...>>; конкретная
 * обёртка над std::function — tim::slot<R, Args...>.
 *
 * \tparam Args Типы аргументов сигнала.
 */
template<typename... Args>
class a_slot
{

public:

    /** Виртуальный деструктор для полиморфного удаления. */
    virtual ~a_slot() = default;

    /**
     * Вызывает слот.
     *
     * \param args Аргументы, передаваемые в слот.
     */
    virtual void invoke(Args... args) const = 0;
};

}
