#include "tim_math_tools.h"

#include <random>


int tim::random_int(int exclusive_max, int inclusive_min)
{
    static std::random_device rd; // Will be used to obtain a seed for the random number engine.
    static std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd().
    std::uniform_int_distribution<> distr(inclusive_min, exclusive_max - 1);

    return distr(gen);
}

tim::float_t tim::random_float(tim::float_t max, tim::float_t min)
{
    static std::random_device rd; // Will be used to obtain a seed for the random number engine.
    static std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd().
    std::uniform_real_distribution<tim::float_t> distr(min, max);

    return distr(gen);
}

tim::float_t tim::random_degree()
{
    return tim::random_float(359.0f);
}
