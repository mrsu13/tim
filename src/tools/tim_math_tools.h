#pragma once

#include "tim_float.h"

#include <cmath>
#include <type_traits>


namespace tim
{

static const tim::float_t PI = std::acos(-1.0f);

template<typename T>
inline T sign(T value);

// Random number generation.
int random_int(int exclusiveMax, int inclusiveMin = 0);

tim::float_t random_float(tim::float_t max, tim::float_t min = 0.0f);

template<typename T>
inline T bound(const T &min, const T &value, const T &max);

template<typename T>
inline T round10(T n);

inline tim::float_t to_degree(tim::float_t radian);
inline tim::float_t to_radian(tim::float_t degree);

inline tim::float_t normalize_degree(tim::float_t degree);
inline tim::float_t natural_degree(tim::float_t degree);

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

/**
 * \return Degree in (-180, 180].
 */
tim::float_t tim::natural_degree(tim::float_t degree)
{
    degree = tim::normalize_degree(degree);
    return degree > 180.0f
                ? degree - 360.0f
                : degree;
}
