#pragma once

#include <cstdint>


namespace tim
{

/**
 * 32-битный RGBA-цвет: 4 байта на канал, по 0..255.
 *
 * Используется терминалом и темами для отрисовки текста/фона.
 * \c a (alpha) применяется ограниченно: VT-эмулятор различает прозрачный
 * (a == 0) и не прозрачный, без промежуточных значений.
 */
struct color
{
    std::uint8_t r = 0; ///< Красный канал, 0..255.
    std::uint8_t g = 0; ///< Зелёный канал, 0..255.
    std::uint8_t b = 0; ///< Синий канал, 0..255.
    std::uint8_t a = 0; ///< Альфа-канал: 0 — прозрачный, >0 — не прозрачный.

    /** Создаёт пустой (полностью прозрачный) цвет с нулевыми каналами. */
    color() = default;

    color(std::uint8_t _r, std::uint8_t _g, std::uint8_t _b, std::uint8_t _a = 0xFF);

    color(const char *html_color);

    static tim::color black();
    static tim::color white();
    static tim::color transparent();

    bool empty() const;
    void clear();

    tim::color text_color() const;
};

}

bool operator==(const tim::color &a, const tim::color &b);
bool operator!=(const tim::color &a, const tim::color &b);
