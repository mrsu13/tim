#pragma once

namespace tim
{

/**
 * Базовый класс-маркер: запрещает копирование производных классов.
 *
 * Наследуйте приватно (`: private tim::non_copyable`), если ваш тип
 * не должен копироваться (RAII-токены, владельцы уникальных ресурсов).
 * Перемещение разрешается явно — non_copyable не определяет
 * move-операторов.
 */
class non_copyable
{

protected:

    /** Конструктор по умолчанию. */
    constexpr non_copyable() = default;
    /** Копирование запрещено. */
    non_copyable(const non_copyable &other) = delete;
    /** Деструктор protected — нельзя delete по указателю на базу. */
    ~non_copyable() = default;

    /** Копи-присваивание запрещено. */
    non_copyable &operator=(const non_copyable &other) = delete;
};

}
