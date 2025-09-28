#pragma once

#include "tim_terminal_theme.h"

#include <cstdarg>
#include <memory>


namespace tim
{

class a_protocol;

namespace p
{

struct a_terminal;

}

class a_terminal
{

public:

    virtual ~a_terminal();

    tim::a_protocol *protocol() const;

    const tim::terminal_theme &theme() const;
    void set_theme(const tim::terminal_theme &theme);

    virtual std::size_t rows() const = 0;
    virtual std::size_t cols() const = 0;
    virtual void clear() = 0;

    virtual std::size_t color_count() const = 0;
    virtual tim::color color(std::size_t index) const = 0;

    virtual void set_color(const tim::color &c) = 0;
    virtual void set_color(std::size_t index) = 0;
    virtual void set_default_color() = 0;
    virtual void set_bg_color(const tim::color &c) = 0;
    virtual void set_bg_color(std::size_t index) = 0;
    virtual void reverse_colors() = 0;
    virtual void reset_colors() = 0;

    int vprintf(const char *format, va_list args);
    int printf(const char *format, ... )
               __attribute__ ((format(printf, 2, 3)));

    int cprintf(const tim::color &text_color,
                const tim::color &bg_color,
                const char *format, ... )
                __attribute__ ((format(printf, 4, 5)));

protected:

    explicit a_terminal(tim::a_protocol *proto);

private:

    std::unique_ptr<tim::p::a_terminal> _d;
};

}
