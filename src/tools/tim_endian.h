#pragma once

#include <cstdint>


namespace tim
{

inline bool is_le()
{
    const int LE_INDICATOR = 1;
    return *reinterpret_cast<const char *>(&LE_INDICATOR) == 1;
}

inline std::uint8_t to_be(const std::uint8_t n)
{
    return n;
}

inline std::uint8_t to_le(const std::uint8_t n)
{
    return n;
}

inline std::uint16_t to_be(const std::uint16_t n)
{
    return is_le()
                ? static_cast<std::uint16_t>((n & 0xff) << 8) | ((n >> 8) & 0xff)
                : n;
}

inline std::uint16_t to_le(const std::uint16_t n)
{
    return is_le()
                ? n
                : static_cast<std::uint16_t>((n & 0xff) << 8) | ((n >> 8) & 0xff);
}

inline std::uint32_t to_be(const std::uint32_t n)
{
    return is_le()
                ? ((n & 0xff000000) >> 24)
                        | ((n & 0x00ff0000) >> 8)
                        | ((n & 0x0000ff00) << 8)
                        | ((n & 0x000000ff) << 24)
                : n;
}

inline std::uint32_t to_le(const std::uint32_t n)
{
    return is_le()
                ? n
                : ((n & 0xff000000) >> 24)
                        | ((n & 0x00ff0000) >> 8)
                        | ((n & 0x0000ff00) << 8)
                        | ((n & 0x000000ff) << 24);
}

}
