#pragma once

#include <type_traits>


namespace tim
{

/**
 * Ограничивает значение интервалом [min, max].
 *
 * \tparam T Арифметический тип.
 * \param min Нижняя граница.
 * \param value Проверяемое значение.
 * \param max Верхняя граница.
 * \return Ближайшее значение из [min, max].
 */
template<typename T>
inline T bound(const T &min, const T &value, const T &max);

}


// Implementation

template<typename T>
T tim::bound(const T &min, const T &value, const T &max)
{
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type.");

    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}
