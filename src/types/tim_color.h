#pragma once

#include <cstdint>


namespace tim
{

struct color
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;

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
