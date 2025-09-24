#pragma once

#include "tim_byte_vector.h"
#include "tim_elide.h"
#include "tim_endian.h"
#include "tim_json.h"
#include "tim_translator.h"
#include "tim_type_traits.h"

#include <cstdarg>
#include <list>
#include <string>
#include <vector>


namespace tim
{

std::wstring to_wstring(const std::string &s);
std::string from_wstring(const std::wstring &ws);

std::string escape_quotes(const std::string &str);
std::string parse_unicode(const std::string &str);

void replace(std::string &str,
             const std::string &old_str,
             const std::string &new_str);

void to_lower(std::string &s);
std::string to_lower(const std::string &s);

void to_upper(std::string &s);
std::string to_upper(const std::string &s);

std::string to_capitalized(const std::string &s);

enum class cs
{
    Sensitive   = 0, ///< \c 0 --- Case sensitive.
    Insensitive = 1 ///< \c 1 ---  Case insensitive.
};

bool starts_with(const std::string &str,
                 const std::string &prefix,
                 const tim::cs cs = tim::cs::Sensitive);
bool ends_with(const std::string &str,
               const std::string &suffix,
               const tim::cs cs = tim::cs::Sensitive);

std::string trim_right(const std::string &s, const char *delimiters = " \t\r\n");
std::string trim_left(const std::string &s, const char *delimiters = " \t\r\n");
std::string trim(const std::string &s, const char *delimiters = " \t\r\n");

std::string simplify(const std::string &s);

void wrap(std::string &s, std::size_t pos);
std::string unwrap(const std::string &s);

std::string strip_pem(const std::string &pem);

int vsprintf(std::string &s, const char *format, va_list args);
std::string vsprintf(const char *format, va_list args);

int sprintf(std::string &s, const char *format, ...)
            __attribute__ ((format(printf, 2, 3)));

std::string sprintf(const char *format, ...)
                    __attribute__ ((format(printf, 1, 2)));

template<typename T>
inline typename std::enable_if_t<std::is_integral_v<T>, std::string> to_string(T value);

template<class T>
inline typename std::enable_if_t<tim::has_to_string_v<T>, std::string> to_string(const T &value);

template<class T>
inline typename std::enable_if_t<std::is_same_v<T, std::string>, std::string> to_string(const T &value);

template<class T>
inline typename std::enable_if_t<std::is_same_v<T, std::filesystem::path>, std::string> to_string(const T &value);

template<typename Unsigned>
std::string to_hex(Unsigned value);

char from_hex(char c);

std::string to_string(const tim::byte_vector &data);
tim::byte_vector from_string(const std::string &s, bool *ok = nullptr);

bool from_string(const std::string &json, nlohmann::json &j);

bool to_bool(const std::string &s, bool *ok = nullptr);
int to_int(const std::string &s, bool *ok = nullptr);
unsigned to_uint(const std::string &s, bool *ok = nullptr);
long long to_long_long(const std::string &s, bool *ok = nullptr);
unsigned long long to_ulong_long(const std::string &s, bool *ok = nullptr);
float to_float(const std::string &s, bool *ok = nullptr);
double to_double(const std::string &s, bool *ok = nullptr);

enum class split_mode
{
    KeepEmptyParts = 0, ///< \с 0 --- If the fragment is empty, keep it in the result.
    SkipEmptyParts = 1  ///< \c 1 --- If the fragment is empty, do not keep it in the result.
};

template<class Collection>
Collection split(const typename Collection::value_type &s,
                 const typename Collection::value_type::value_type *delimiters = " \t\r\n",
                 tim::split_mode behavior = tim::split_mode::SkipEmptyParts);

template<class String>
inline std::vector<String> split_v(const String &s,
                                   const typename String::value_type *delimiters = " \t\r\n",
                                   tim::split_mode behavior = tim::split_mode::SkipEmptyParts);

template<class String>
inline std::list<String> split_l(const String &s,
                                 const typename String::value_type *delimiters = " \t\r\n",
                                 tim::split_mode behavior = tim::split_mode::SkipEmptyParts);

std::string member_name(const char *name);

std::string to_dotted(const std::string &snake);
std::string to_snake(const std::string &dotted);

std::string centered(const std::string &str, std::size_t width);

std::string elided(const std::string &str, std::size_t width, tim::elide el = tim::elide::Right);

enum class text_align
{
    Left    = 0, ///< \c 0 --- Align with the left edge.
    Right   = 1, ///< \c 1 --- Align with the right edge.
    Center  = 2, ///< \c 2 --- Center horizontally in the available space.
    Justify = 3  ///< \c 3 --- Justify the text in the available space.
};

std::string aligned(const std::string &str,
                    tim::text_align al = tim::text_align::Justify,
                    std::size_t width = 80);

template<class S1, class S2>
bool equal(const S1 &str1,
           const S2 &str2,
           const tim::cs cs = tim::cs::Sensitive);

bool strcasecmp(const char *s1, const char *s2);

inline const char *yes_no(bool f);
inline const char *na();

}


// Implementation

template<typename T>
typename std::enable_if_t<std::is_integral_v<T>, std::string> tim::to_string(T value)
{
    return std::to_string(value);
}

template<class T>
typename std::enable_if_t<tim::has_to_string_v<T>, std::string> tim::to_string(const T &value)
{
    return value.to_string();
}

template<class T>
typename std::enable_if_t<std::is_same_v<T, std::string>, std::string> tim::to_string(const T &value)
{
    return value;
}

template<class T>
typename std::enable_if_t<std::is_same_v<T, std::filesystem::path>, std::string> tim::to_string(const T &value)
{
    return value.string();
}

/**
 * \param value unsigned integer.
 * \return Hex representation of the \a value as a string.
 */
template<typename Unsigned>
std::string tim::to_hex(Unsigned value)
{
    static_assert(std::is_unsigned_v<Unsigned>,
                  "Unsigned must be of unsigned integer type.");

    static const char *const DIGITS = "0123456789abcdef";

    value = tim::to_be(value);

    const char *p = reinterpret_cast<const char *>(&value);

    std::string s;
    s.reserve(sizeof(value) * 2);

    for (unsigned i = 0; i < sizeof(Unsigned); ++i)
    {
        unsigned int j = (p[i] >> 4) & 0xF;
        s.push_back(DIGITS[j]);
        j = p[i] & 0xF;
        s.push_back(DIGITS[j]);
    }

    return s;
}

template<class Collection>
Collection tim::split(const typename Collection::value_type &s,
                      const typename Collection::value_type::value_type *delimiters,
                      tim::split_mode behavior)
{
    Collection collection;

    // Special case - delimiters are empty. Split to individual characters
    if (!delimiters
            || !*delimiters)
    {
        for (const typename Collection::value_type::value_type c: s)
            collection.emplace_back(typename Collection::value_type(&c, 1));
        return collection;
    }

    // Beginning of field.
    std::size_t j = s.find_first_not_of(delimiters);

    // String consists only from delimiters.
    if (j == Collection::value_type::npos)
    {
        if (behavior == tim::split_mode::KeepEmptyParts)
            collection.resize(s.size());
        return collection;
    }
    if (j > 0
            && behavior == tim::split_mode::KeepEmptyParts)
        collection.resize(j);

    // Beginning of delimiters.
    std::size_t i = s.find_first_of(delimiters, j);

    while (i != Collection::value_type::npos)
    {
        collection.emplace_back(s.substr(j, i - j));
        j = s.find_first_not_of(delimiters, i);

        // Rest of string - delimiters only.
        if (j == Collection::value_type::npos)
        {
            if (behavior == tim::split_mode::KeepEmptyParts)
                collection.resize(collection.size() + s.size() - i);
            return collection;
        }

        if (j > i + 1
                && behavior == tim::split_mode::KeepEmptyParts)
            collection.resize(collection.size() + j - (i + 1));

        // Next beginning of delimiters.
        i = s.find_first_of(delimiters, j);
    }

    collection.emplace_back(s.substr(j));
    return collection;
}

template<class String>
std::vector<String> tim::split_v(const String &s,
                                 const typename String::value_type *delimiters,
                                 tim::split_mode behavior)
{
    return tim::split<std::vector<String>>(s, delimiters, behavior);
}

template<class String>
std::list<String> tim::split_l(const String &s,
                               const typename String::value_type *delimiters,
                               tim::split_mode behavior)
{
    return tim::split<std::list<String>>(s, delimiters, behavior);
}

template<class S1, class S2>
bool tim::equal(const S1 &str1,
                const S2 &str2,
                const tim::cs cs)
{
    if (str1.size() != str2.size())
        return false;

    return cs == tim::cs::Sensitive
                ? str1 == str2
                : std::equal(str2.cbegin(), str2.cend(),
                              str1.cbegin(),
                              [](char a, char b)
                              {
                                    return std::tolower(a) == std::tolower(b);
                              });
}

const char *tim::yes_no(bool f)
{
    return f
                ? TIM_TR("Yes"_en, "Да"_ru)
                : TIM_TR("No"_en, "Нет"_ru);
}

const char *tim::na()
{
    return TIM_TR("N/A"_en, "Не задано"_ru);
}
