#pragma once

#include <cstdint>


namespace tim
{

/**
 * Определяет порядок байт текущей платформы во время выполнения.
 *
 * \return true, если система little-endian.
 */
inline bool is_le()
{
    const int LE_INDICATOR = 1;
    return *reinterpret_cast<const char *>(&LE_INDICATOR) == 1;
}

/** Преобразует \a n в big-endian. \param n 8-битное беззнаковое значение.
 *  \return Тот же \a n (порядок байт не имеет значения). */
inline std::uint8_t to_be(const std::uint8_t n)
{
    return n;
}

/** Преобразует \a n в little-endian. \param n 8-битное значение.
 *  \return Тот же \a n. */
inline std::uint8_t to_le(const std::uint8_t n)
{
    return n;
}

/**
 * Преобразует 16-битное значение в big-endian.
 *
 * \param n 16-битное беззнаковое значение в host-порядке.
 * \return То же значение в big-endian.
 */
inline std::uint16_t to_be(const std::uint16_t n)
{
    return is_le()
                ? static_cast<std::uint16_t>((n & 0xff) << 8) | ((n >> 8) & 0xff)
                : n;
}

/**
 * Преобразует 16-битное значение в little-endian.
 *
 * \param n 16-битное беззнаковое значение в host-порядке.
 * \return То же значение в little-endian.
 */
inline std::uint16_t to_le(const std::uint16_t n)
{
    return is_le()
                ? n
                : static_cast<std::uint16_t>((n & 0xff) << 8) | ((n >> 8) & 0xff);
}

/**
 * Преобразует 32-битное значение в big-endian.
 *
 * \param n 32-битное беззнаковое значение в host-порядке.
 * \return То же значение в big-endian.
 */
inline std::uint32_t to_be(const std::uint32_t n)
{
    return is_le()
                ? ((n & 0xff000000) >> 24)
                        | ((n & 0x00ff0000) >> 8)
                        | ((n & 0x0000ff00) << 8)
                        | ((n & 0x000000ff) << 24)
                : n;
}

/**
 * Преобразует 32-битное значение в little-endian.
 *
 * \param n 32-битное беззнаковое значение в host-порядке.
 * \return То же значение в little-endian.
 */
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
