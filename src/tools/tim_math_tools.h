#pragma once

#include "tim_float.h"

#include <cmath>
#include <type_traits>


namespace tim
{

/** Число пи (вычисляется один раз через std::acos). */
static const tim::float_t PI = std::acos(-1.0f);

/**
 * Возвращает знак числа.
 *
 * \tparam T Арифметический тип (целый или float).
 * \param value Значение.
 * \return -1, 0 или +1 в зависимости от знака \a value.
 */
template<typename T>
inline T sign(T value);

/**
 * Случайное целое число.
 *
 * \param exclusiveMax Верхняя граница (не включается).
 * \param inclusiveMin Нижняя граница (включается). По умолчанию 0.
 * \return Случайное число в [inclusiveMin, exclusiveMax).
 */
int random_int(int exclusiveMax, int inclusiveMin = 0);

/**
 * Случайное число с плавающей точкой.
 *
 * \param max Верхняя граница (не включается).
 * \param min Нижняя граница (включается). По умолчанию 0.0.
 * \return Случайное float в [min, max).
 */
tim::float_t random_float(tim::float_t max, tim::float_t min = 0.0f);

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

/**
 * Округляет float до десятых.
 *
 * \tparam T Тип с плавающей точкой.
 * \param n Значение.
 * \return \a n с округлением до 0.1.
 */
template<typename T>
inline T round10(T n);

/**
 * Радианы → градусы.
 *
 * \param radian Угол в радианах.
 * \return Тот же угол в градусах.
 */
inline tim::float_t to_degree(tim::float_t radian);

/**
 * Градусы → радианы.
 *
 * \param degree Угол в градусах.
 * \return Тот же угол в радианах.
 */
inline tim::float_t to_radian(tim::float_t degree);

/**
 * Приводит угол к [0, 360).
 *
 * \param degree Любой угол в градусах.
 * \return Эквивалентный угол в [0, 360).
 */
inline tim::float_t normalize_degree(tim::float_t degree);

/**
 * Приводит угол к "натуральному" диапазону (-180, 180].
 *
 * \param degree Любой угол в градусах.
 * \return Эквивалентный угол в (-180, 180].
 */
inline tim::float_t natural_degree(tim::float_t degree);

/**
 * Случайный угол в градусах.
 *
 * \return Случайное значение в [0, 360).
 */
tim::float_t random_degree();

}


// Implementation

template<typename T>
T tim::sign(T value)
{
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type.");
    return (T(0) < value) - (value < T(0));
}

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

template<typename T>
T tim::round10(T n)
{
    static_assert(std::is_floating_point_v<T>, "T must be a floating point type.");

    return std::floor(10.0f * n + 0.5f) / 10.0f;
}

tim::float_t tim::to_degree(tim::float_t radian)
{
    return 180.0f / tim::PI * radian;
}

tim::float_t tim::to_radian(tim::float_t degree)
{
    return degree * tim::PI / 180.0f;
}

tim::float_t tim::normalize_degree(tim::float_t degree)
{
    return degree + std::ceil(-degree / 360.0f) * 360.0f;
}

tim::float_t tim::natural_degree(tim::float_t degree)
{
    degree = tim::normalize_degree(degree);
    return degree > 180.0f
                ? degree - 360.0f
                : degree;
}
