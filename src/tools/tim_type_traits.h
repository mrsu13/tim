#pragma once

#include <string>
#include <type_traits>


namespace tim
{

/**
 * Безопасное преобразование значения From → To. Отказная ветвь:
 * если From не приводится к To по правилам C++, возвращает false
 * и оставляет \a to нетронутым.
 *
 * \tparam From Тип источника.
 * \tparam To Тип назначения.
 * \param from Значение источника.
 * \param to Сюда записывается результат при возможности приведения.
 * \return false (для неконвертируемой пары).
 */
template<typename From, typename To>
inline typename std::enable_if_t<!std::is_convertible_v<From, To>, bool>
                    convert(const From &from, To &to)
{
    (void) from;
    (void) to;
    return false;
}

/**
 * Конвертируемая пара: приводит From к To и кладёт в \a to.
 *
 * \tparam From Тип источника.
 * \tparam To Тип назначения, конвертируемый из From.
 * \param from Значение источника.
 * \param to Сюда записывается приведённое значение.
 * \return true.
 */
template<typename From, typename To>
inline typename std::enable_if_t<std::is_convertible_v<From, To>, bool>
                    convert(const From &from, To &to)
{
    to = (To)from;
    return true;
}


/**
 * SFINAE-проверка: имеет ли тип C метод `to_string()`, возвращающий
 * std::string.
 *
 * \tparam C Проверяемый тип.
 */
template<class C>
struct has_to_string
{

private:

    /**
     * Хвост SFINAE: пытается вызвать to_string() и сравнить тип
     * возврата с std::string.
     */
    template<typename T>
    static constexpr auto check(T *)
        -> typename
            std::is_same<
                decltype(std::declval<T>().to_string()),
                std::string
            >::type;

    /** Перегруз для случаев, когда to_string() недоступен. */
    template<typename>
    static constexpr std::false_type check(...);

    /** Полученный тип-результат (std::true_type или std::false_type). */
    typedef decltype(check<C>(nullptr)) type;

public:

    /** true, если у типа есть to_string() → std::string. */
    static constexpr bool value = type::value;

};

/** Сахар: has_to_string<T>::value через variable template. */
template<typename T>
inline constexpr bool has_to_string_v = tim::has_to_string<T>::value;

}
