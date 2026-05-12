#include "tim_math_tools.h"

#include <random>


/**
 * Случайное целое в [inclusive_min, exclusive_max). Использует
 * mt19937, разово засеянный random_device.
 */
int tim::random_int(int exclusive_max, int inclusive_min)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(inclusive_min, exclusive_max - 1);

    return distr(gen);
}

/**
 * Случайное float в [min, max). Использует тот же mt19937, что и
 * random_int (но со своим distribution).
 */
tim::float_t tim::random_float(tim::float_t max, tim::float_t min)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<tim::float_t> distr(min, max);

    return distr(gen);
}

/**
 * Случайный угол в [0, 359). Тонкая обёртка над random_float().
 */
tim::float_t tim::random_degree()
{
    return tim::random_float(359.0f);
}
