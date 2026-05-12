#pragma once

// Минимальный харнесс для тестов TIM.
//
// TIM_TEST_CASE(name) { ... TIM_CHECK(...); ... }
//
// Все тестовые функции автоматически регистрируются через статическую
// инициализацию. Тестовый main() (см. test_main.cpp) обходит реестр и
// печатает OK/FAIL для каждого случая, итог по всем — в конце.

#include <cstddef>
#include <iostream>
#include <vector>


namespace tim::test
{

struct case_t
{
    const char *name;
    void (*fn)();
};

inline std::vector<case_t> &all()
{
    static std::vector<case_t> v;
    return v;
}

inline std::size_t &total_failures()
{
    static std::size_t n = 0;
    return n;
}

inline std::size_t &current_case_failures()
{
    static std::size_t n = 0;
    return n;
}

}


#define TIM_TEST_CASE(name)                                              \
    static void tim_test_##name();                                       \
    namespace                                                            \
    {                                                                    \
        struct tim_test_reg_##name                                       \
        {                                                                \
            tim_test_reg_##name()                                        \
            {                                                            \
                tim::test::all().push_back({ #name, &tim_test_##name }); \
            }                                                            \
        };                                                               \
        static tim_test_reg_##name tim_test_reg_##name##_inst;           \
    }                                                                    \
    static void tim_test_##name()

#define TIM_CHECK(expr)                                                \
    do                                                                 \
    {                                                                  \
        if (!(expr))                                                   \
        {                                                              \
            std::cout << "\n  FAIL: " #expr                            \
                      << " at " << __FILE__ << ':' << __LINE__;        \
            ++tim::test::current_case_failures();                      \
            ++tim::test::total_failures();                              \
        }                                                              \
    } while (0)
