#pragma once

#include "tim_a_signal.h"
#include "tim_signal_connection.h"
#include "tim_slot.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>


namespace tim
{

/**
 * Шаблон сигнала для произвольного списка аргументов слотов.
 *
 * Подписчики (слоты) подключаются через connect() и получают токен
 * tim::signal_connection — при его разрушении подписка автоматически
 * отзывается. Эмиссия через operator() безопасна к (от)подключению
 * слотов изнутри слота: перед обходом снимается снимок идентификаторов.
 *
 * \tparam Args Типы аргументов, с которыми сигнал испускается.
 */
template<typename... Args>
class signal : public tim::a_signal
{

public:

    /** Создаёт пустой сигнал без слотов. */
    signal();

    /**
     * Подключает слот, возвращающий значение типа R.
     *
     * \tparam R Тип возвращаемого значения слота (обычно void).
     * \param fn Функция/лямбда слота.
     * \return RAII-токен подписки.
     */
    template<typename R>
    [[nodiscard]] tim::signal_connection connect(std::function<R (Args...)> fn);

    /**
     * Подключает void-слот.
     *
     * \param fn Функция/лямбда слота.
     * \return RAII-токен подписки.
     */
    [[nodiscard]] tim::signal_connection connect(std::function<void (Args...)> fn);

    /**
     * Отключает слот по идентификатору. Вызывается деструктором
     * signal_connection; редко нужно напрямую.
     *
     * \param connection_id Идентификатор подключения.
     * \return true, если слот был и удалён.
     */
    bool disconnect(std::size_t connection_id) override;

    /**
     * Испускает сигнал, последовательно вызывая все слоты.
     * Снимает снимок идентификаторов перед итерацией — слоту разрешено
     * (от)подключаться во время эмиссии без UB.
     *
     * \param args Аргументы, передаваемые в каждый слот.
     */
    void operator()(Args... args) const;

private:

    /**
     * Возвращает следующий уникальный (на тип сигнала) идентификатор
     * подключения. Глобальный счётчик инстанцирования.
     *
     * \return Уникальный id.
     */
    inline static std::size_t next_id();

    /** Карта id → слот. unordered_map для O(1)-вставки/удаления. */
    using slot_map = std::unordered_map<std::size_t, std::unique_ptr<tim::a_slot<Args...>>>;
    /** Хранилище подключённых слотов. */
    slot_map _slots;
};

}


// Реализация

// Открытые

/**
 * Конструктор: пустой набор слотов.
 */
template<typename... Args>
tim::signal<Args...>::signal()
    : tim::a_signal()
    , _slots()
{
}

/**
 * Создаёт slot<R, Args...>, кладёт в _slots под новым id и возвращает
 * токен, привязанный к (this, id).
 */
template<typename... Args>
template<typename R>
tim::signal_connection tim::signal<Args...>::connect(std::function<R (Args...)> fn)
{
    const std::size_t id = next_id();
    _slots[id] = std::move(std::make_unique<tim::slot<R, Args...>>(fn));
    return tim::signal_connection(std::pair<tim::a_signal *, std::size_t>{ this, id });
}

/**
 * Перегруз void-варианта: делегирует connect<void>().
 */
template<typename... Args>
tim::signal_connection tim::signal<Args...>::connect(std::function<void (Args...)> fn)
{
    return connect<void>(fn);
}

/**
 * Удаляет слот из _slots; map::erase возвращает 0/1.
 */
template<typename... Args>
bool tim::signal<Args...>::disconnect(std::size_t connection_id)
{
    return _slots.erase(connection_id);
}

/**
 * Снимает снимок id подключений (чтобы слот мог безопасно
 * (от)подключиться во время эмиссии), затем последовательно вызывает
 * слоты по id.
 */
template<typename... Args>
void tim::signal<Args...>::operator()(Args... args) const
{
    std::vector<std::size_t> ids;
    ids.reserve(_slots.size());
    for (const typename slot_map::value_type &pair: _slots)
        ids.push_back(pair.first);

    for (std::size_t id: ids)
    {
        const typename slot_map::const_iterator it = _slots.find(id);
        if (it != _slots.cend())
            it->second->invoke(args...);
    }
}


// Закрытые

/**
 * Статический счётчик id; каждый инстанс шаблона имеет свой.
 */
template<typename... Args>
std::size_t tim::signal<Args...>::next_id()
{
    static std::size_t id = 0;
    return id++;
}
