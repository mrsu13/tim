#pragma once

#include <cstdint>
#include <unordered_map>


namespace tim
{

using translation = std::pair<std::int64_t, const char *>;
using translations = std::unordered_map<std::int64_t, const char *>;

}

inline constexpr tim::translation operator ""_en(const char *s, std::size_t n)
#ifdef __GNUC__
    __attribute__((nonnull(1)))
#endif
;

inline constexpr tim::translation operator ""_ru(const char *s, std::size_t n)
#ifdef __GNUC__
    __attribute__((nonnull(1)))
#endif
;


inline constexpr tim::translation operator ""_en(const char *s, std::size_t n)
{
    (void) n;

    // We cannot use tim::language::English here because we cannot include tim_language.h.
    return tim::translation{ 1, s };
}

inline constexpr tim::translation operator ""_ru(const char *s, std::size_t n)
{
    (void) n;

    // We cannot use tim::language::Russian here because we cannot include tim_language.h.
    return tim::translation{ 2, s };
}
