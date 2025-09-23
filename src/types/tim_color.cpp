#include "tim_color.h"

#include "tim_trace.h"
#include "tim_translator.h"

#include <cassert>
#include <cstdio>


// Public

tim::color::color(std::uint8_t _r, std::uint8_t _g, std::uint8_t _b, std::uint8_t _a)
    : r(_r)
    , g(_g)
    , b(_b)
    , a(_a)
{
}

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

    TIM_TRACE(Error,
              TIM_TR("Invalid HTML color '%s'."_en,
                     "Недопустимый цвет HTML '%s'."_ru),
              html_color);
}

bool tim::color::empty() const
{
    return (!r && !g && !b) || a == 0;
}

void tim::color::clear()
{
    r = 0;
    g = 0;
    b = 0;
    a = 0;
}

bool operator==(const tim::color &a, const tim::color &b)
{
    return a.r == b.r
                && a.g == b.g
                && a.b == b.b
                && a.a == b.a;
}

bool operator!=(const tim::color &a, const tim::color &b)
{
    return a.r != b.r
                || a.g != b.g
                || a.b != b.b
                || a.a != b.a;
}
