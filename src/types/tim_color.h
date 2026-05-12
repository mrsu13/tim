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

    /**
     * Создаёт цвет из значений каналов.
     *
     * \param _r Красный.
     * \param _g Зелёный.
     * \param _b Синий.
     * \param _a Альфа (по умолчанию непрозрачный, 0xFF).
     */
    color(std::uint8_t _r, std::uint8_t _g, std::uint8_t _b, std::uint8_t _a = 0xFF);

    /**
     * Создаёт цвет из строки в формате "#RRGGBB" или "#RRGGBBAA".
     *
     * \param html_color Строка с HTML-цветом; начинается с '#'.
     */
    color(const char *html_color);

    /** \return Чёрный цвет (#000000FF). */
    static tim::color black();
    /** \return Белый цвет (#FFFFFFFF). */
    static tim::color white();
    /** \return Прозрачный цвет (alpha == 0). */
    static tim::color transparent();

    /** \return true, если цвет нулевой (все каналы 0, включая alpha). */
    bool empty() const;
    /** Обнуляет все каналы, делая цвет пустым/прозрачным. */
    void clear();

    /**
     * Возвращает авто-контрастный цвет текста для этого фона:
     * чёрный для светлых фонов, белый для тёмных.
     *
     * \return Цвет текста, читаемый на этом фоне.
     */
    tim::color text_color() const;
};

}

/** Покомпонентное сравнение цветов. */
bool operator==(const tim::color &a, const tim::color &b);
/** Покомпонентное сравнение цветов. */
bool operator!=(const tim::color &a, const tim::color &b);
