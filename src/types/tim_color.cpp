#include "tim_color.h"

#include "tim_trace.h"
#include "tim_translator.h"

#include <cassert>
#include <cmath>
#include <cstdio>


// Public

/**
 * Создаёт цвет из значений каналов.
 *
 * \param _r Красный.
 * \param _g Зелёный.
 * \param _b Синий.
 * \param _a Альфа (по умолчанию непрозрачный, 0xFF).
 */
tim::color::color(std::uint8_t _r, std::uint8_t _g, std::uint8_t _b, std::uint8_t _a)
    : r(_r)
    , g(_g)
    , b(_b)
    , a(_a)
{
}

/**
 * Создаёт цвет из строки в формате "#RRGGBB" или "#RRGGBBAA".
 *
 * \param html_color Строка с HTML-цветом; начинается с '#'.
 */
tim::color::color(const char *html_color)
{
    assert(html_color && *html_color);

    unsigned _r, _g, _b, _a;
    switch (std::sscanf(html_color, "#%02X%02X%02X%02X", &_r, &_g, &_b, &_a))
    {
        case 4: // RGBA
            r = (std::uint8_t)_r;
            g = (std::uint8_t)_g;
            b = (std::uint8_t)_b;
            a = (std::uint8_t)_a;
            return;

        case 3: // RGB
            r = (std::uint8_t)_r;
            g = (std::uint8_t)_g;
            b = (std::uint8_t)_b;
            a = 0xFF;
            return;

        default:
            break;
    }

    switch (std::sscanf(html_color, "#%02x%02x%02x%02x", &_r, &_g, &_b, &_a))
    {
        case 4: // RGBA
            r = (std::uint8_t)_r;
            g = (std::uint8_t)_g;
            b = (std::uint8_t)_b;
            a = (std::uint8_t)_a;
            return;

        case 3: // RGB
            r = (std::uint8_t)_r;
            g = (std::uint8_t)_g;
            b = (std::uint8_t)_b;
            a = 0xFF;
            return;

        default:
            break;
    }

    TIM_TRACE(error,
              TIM_TR("Invalid HTML color '%s'."_en,
                     "Недопустимый цвет HTML '%s'."_ru),
              html_color);
}

/** \return Чёрный цвет (#000000FF). */
tim::color tim::color::black()
{
    return tim::color{ 0, 0, 0, 0xFF };
}

/** \return Белый цвет (#FFFFFFFF). */
tim::color tim::color::white()
{
    return tim::color{ 0xFF, 0xFF, 0xFF, 0xFF };
}

/** \return Прозрачный цвет (alpha == 0). */
tim::color tim::color::transparent()
{
    return tim::color{};
}

/** \return true, если цвет нулевой (все каналы 0, включая alpha). */
bool tim::color::empty() const
{
    return (!r && !g && !b) || a == 0;
}

/** Обнуляет все каналы, делая цвет пустым/прозрачным. */
void tim::color::clear()
{
    r = 0;
    g = 0;
    b = 0;
    a = 0;
}

/**
 * Возвращает авто-контрастный цвет текста для этого фона:
 * чёрный для светлых фонов, белый для тёмных.
 *
 * \return Цвет текста, читаемый на этом фоне.
 */
tim::color tim::color::text_color() const
{
    const float luminosity = std::sqrt(
        r * r * 0.299f // Красный
            + g * g * 0.587f // Зелёный
            + b * b * 0.114f); // Синий

    return luminosity > (186.0f / 255.0f)
                ? tim::color::black()
                : tim::color::white();
}

/** Покомпонентное сравнение цветов. */
bool operator==(const tim::color &a, const tim::color &b)
{
    return a.r == b.r
                && a.g == b.g
                && a.b == b.b
                && a.a == b.a;
}

/** Покомпонентное сравнение цветов. */
bool operator!=(const tim::color &a, const tim::color &b)
{
    return a.r != b.r
                || a.g != b.g
                || a.b != b.b
                || a.a != b.a;
}
