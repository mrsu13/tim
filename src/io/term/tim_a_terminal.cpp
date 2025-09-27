#include "tim_a_terminal.h"

#include "tim_a_terminal_p.h"

#include "tim_string_tools.h"
#include "tim_trace.h"

#include <cassert>


// Public

tim::a_terminal::~a_terminal() = default;

const tim::vt_theme &tim::a_terminal::theme() const
{
    return _d->_theme;
}

void tim::a_terminal::set_theme(const tim::vt_theme &theme)
{
    _d->_theme = theme;
}

int tim::a_terminal::vprintf(const char *format, va_list args)
{
    assert(format && *format);

    std::string s;

    va_list args_copy;
    va_copy(args_copy, args);
    const int n = tim::vsprintf(s, format, args);
    va_end(args_copy);

    if (n > 0)
        write(s.c_str(), n);

    return n;
}

int tim::a_terminal::printf(const char *format, ... )
{
    assert(format && *format);

    va_list args;
    va_start(args, format);
    const int n = tim::a_terminal::vprintf(format, args);
    va_end(args);

    return n;
}

int tim::a_terminal::cprintf(const tim::color &text_color,
                             const tim::color &bg_color,
                             const char *format, ... )
{
    assert(format && *format);

    set_color(text_color);
    set_bg_color(bg_color);

    va_list args;
    va_start(args, format);
    const int n = tim::a_terminal::vprintf(format, args);
    va_end(args);

    if (!text_color.empty()
            || !bg_color.empty())
        reset_colors();

    return n;
}


// Protected

tim::a_terminal::a_terminal(mg_connection *c)
    : tim::a_io_device(c)
    , _d(new tim::p::a_terminal())
{
}
