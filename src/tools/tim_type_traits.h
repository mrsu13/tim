#pragma once

#include <string>
#include <type_traits>


namespace tim
{

template<typename From, typename To>
inline typename std::enable_if_t<!std::is_convertible_v<From, To>, bool>
                    convert(const From &from, To &to)
{
    (void) from;
    (void) to;
    return false;
}

template<typename From, typename To>
inline typename std::enable_if_t<std::is_convertible_v<From, To>, bool>
                    convert(const From &from, To &to)
{
    to = (To)from;
    return true;
}


template<class C>
struct has_to_string
{

private:

    template<typename T>

    static constexpr auto check(T *)
        -> typename
            std::is_same<
                decltype(std::declval<T>().to_string()), // Attempt to call it and see if the return type is correct.
                std::string
            >::type;

    template<typename>
    static constexpr std::false_type check(...);

    typedef decltype(check<C>(nullptr)) type;

public:

    static constexpr bool value = type::value;

};

template<typename T>
inline constexpr bool has_to_string_v = tim::has_to_string<T>::value;

}
