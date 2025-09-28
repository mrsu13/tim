#include "tim_vt.h"

#include "tim_vt_p.h"

#include "tim_a_protocol.h"
#include "tim_string_tools.h"
#include "tim_trace.h"

#include "utf8/utf8.h"

#include <cassert>


namespace tim
{

// See <https://gist.github.com/MicahElliott/719710>.
static const tim::color VT_PALETTE256[] =
{
    "#000000",
    "#800000",
    "#008000",
    "#808000",
    "#000080",
    "#800080",
    "#008080",
    "#C0C0C0",

    // Equivalent "bright" versions of original 8 colors.
    "#808080",
    "#FF0000",
    "#00FF00",
    "#FFFF00",
    "#0000FF",
    "#FF00FF",
    "#00FFFF",
    "#FFFFFF",

    // Strictly ascending.
    "#000000",
    "#00005F",
    "#000087",
    "#0000AF",
    "#0000D7",
    "#0000FF",
    "#005F00",
    "#005F5F",
    "#005F87",
    "#005FAF",
    "#005FD7",
    "#005FFF",
    "#008700",
    "#00875F",
    "#008787",
    "#0087AF",
    "#0087D7",
    "#0087FF",
    "#00AF00",
    "#00AF5F",
    "#00AF87",
    "#00AFAF",
    "#00AFD7",
    "#00AFFF",
    "#00D700",
    "#00D75F",
    "#00D787",
    "#00D7AF",
    "#00D7D7",
    "#00D7FF",
    "#00FF00",
    "#00FF5F",
    "#00FF87",
    "#00FFAF",
    "#00FFD7",
    "#00FFFF",
    "#5F0000",
    "#5F005F",
    "#5F0087",
    "#5F00AF",
    "#5F00D7",
    "#5F00FF",
    "#5F5F00",
    "#5F5F5F",
    "#5F5F87",
    "#5F5FAF",
    "#5F5FD7",
    "#5F5FFF",
    "#5F8700",
    "#5F875F",
    "#5F8787",
    "#5F87AF",
    "#5F87D7",
    "#5F87FF",
    "#5FAF00",
    "#5FAF5F",
    "#5FAF87",
    "#5FAFAF",
    "#5FAFD7",
    "#5FAFFF",
    "#5FD700",
    "#5FD75F",
    "#5FD787",
    "#5FD7AF",
    "#5FD7D7",
    "#5FD7FF",
    "#5FFF00",
    "#5FFF5F",
    "#5FFF87",
    "#5FFFAF",
    "#5FFFD7",
    "#5FFFFF",
    "#870000",
    "#87005F",
    "#870087",
    "#8700AF",
    "#8700D7",
    "#8700FF",
    "#875F00",
    "#875F5F",
    "#875F87",
    "#875FAF",
    "#875FD7",
    "#875FFF",
    "#878700",
    "#87875F",
    "#878787",
    "#8787AF",
    "#8787D7",
    "#8787FF",
    "#87AF00",
    "#87AF5F",
    "#87AF87",
    "#87AFAF",
    "#87AFD7",
    "#87AFFF",
    "#87D700",
    "#87D75F",
    "#87D787",
    "#87D7AF",
    "#87D7D7",
    "#87D7FF",
    "#87FF00",
    "#87FF5F",
    "#87FF87",
    "#87FFAF",
    "#87FFD7",
    "#87FFFF",
    "#AF0000",
    "#AF005F",
    "#AF0087",
    "#AF00AF",
    "#AF00D7",
    "#AF00FF",
    "#AF5F00",
    "#AF5F5F",
    "#AF5F87",
    "#AF5FAF",
    "#AF5FD7",
    "#AF5FFF",
    "#AF8700",
    "#AF875F",
    "#AF8787",
    "#AF87AF",
    "#AF87D7",
    "#AF87FF",
    "#AFAF00",
    "#AFAF5F",
    "#AFAF87",
    "#AFAFAF",
    "#AFAFD7",
    "#AFAFFF",
    "#AFD700",
    "#AFD75F",
    "#AFD787",
    "#AFD7AF",
    "#AFD7D7",
    "#AFD7FF",
    "#AFFF00",
    "#AFFF5F",
    "#AFFF87",
    "#AFFFAF",
    "#AFFFD7",
    "#AFFFFF",
    "#D70000",
    "#D7005F",
    "#D70087",
    "#D700AF",
    "#D700D7",
    "#D700FF",
    "#D75F00",
    "#D75F5F",
    "#D75F87",
    "#D75FAF",
    "#D75FD7",
    "#D75FFF",
    "#D78700",
    "#D7875F",
    "#D78787",
    "#D787AF",
    "#D787D7",
    "#D787FF",
    "#D7AF00",
    "#D7AF5F",
    "#D7AF87",
    "#D7AFAF",
    "#D7AFD7",
    "#D7AFFF",
    "#D7D700",
    "#D7D75F",
    "#D7D787",
    "#D7D7AF",
    "#D7D7D7",
    "#D7D7FF",
    "#D7FF00",
    "#D7FF5F",
    "#D7FF87",
    "#D7FFAF",
    "#D7FFD7",
    "#D7FFFF",
    "#FF0000",
    "#FF005F",
    "#FF0087",
    "#FF00AF",
    "#FF00D7",
    "#FF00FF",
    "#FF5F00",
    "#FF5F5F",
    "#FF5F87",
    "#FF5FAF",
    "#FF5FD7",
    "#FF5FFF",
    "#FF8700",
    "#FF875F",
    "#FF8787",
    "#FF87AF",
    "#FF87D7",
    "#FF87FF",
    "#FFAF00",
    "#FFAF5F",
    "#FFAF87",
    "#FFAFAF",
    "#FFAFD7",
    "#FFAFFF",
    "#FFD700",
    "#FFD75F",
    "#FFD787",
    "#FFD7AF",
    "#FFD7D7",
    "#FFD7FF",
    "#FFFF00",
    "#FFFF5F",
    "#FFFF87",
    "#FFFFAF",
    "#FFFFD7",
    "#FFFFFF",

    // Gray-scale range.
    "#080808",
    "#121212",
    "#1C1C1C",
    "#262626",
    "#303030",
    "#3A3A3A",
    "#444444",
    "#4E4E4E",
    "#585858",
    "#626262",
    "#6C6C6C",
    "#767676",
    "#808080",
    "#8A8A8A",
    "#949494",
    "#9E9E9E",
    "#A8A8A8",
    "#B2B2B2",
    "#BCBCBC",
    "#C6C6C6",
    "#D0D0D0",
    "#DADADA",
    "#E4E4E4",
    "#EEEEEE"
};

}


// Public

tim::vt::vt(tim::a_protocol *proto)
    : tim::a_terminal(proto)
    , _d(new tim::p::vt(this))
{
}

tim::vt::~vt() = default;

std::size_t tim::vt::rows() const
{
    return std::max(10U, _d->_rows);
}

std::size_t tim::vt::cols() const
{
    return std::max(20U, _d->_cols);
}

void tim::vt::clear()
{
    static const char cmd[] = "\x1b[H\x1b[2J";
    protocol()->write(cmd, sizeof(cmd) - 1);
}

std::size_t tim::vt::color_count() const
{
    return sizeof(tim::VT_PALETTE256) / sizeof(tim::VT_PALETTE256[0]);
}

tim::color tim::vt::color(std::size_t index) const
{
    return tim::VT_PALETTE256[index % color_count()];
}

/**
 * Set text color.
 */
void tim::vt::set_color(const tim::color &c)
{
    if (!c.empty())
        tim::vt::printf("\x1b[38;2;%u;%u;%um", c.r, c.g, c.b);
}

void tim::vt::set_color(std::size_t index)
{
    set_color(color(index));
}

void tim::vt::set_default_color()
{
    protocol()->write("\x1b[39m", 5);
}

void tim::vt::set_bg_color(const tim::color &c)
{
    if (!c.empty())
        tim::vt::printf("\x1b[48;2;%u;%u;%um", c.r, c.g, c.b);
}

void tim::vt::set_bg_color(std::size_t index)
{
    set_bg_color(color(index));
}

void tim::vt::reverse_colors()
{
    protocol()->write("\x1b[7m", 4);
}

/**
 * Reset text and background colors to their default values.
 */
void tim::vt::reset_colors()
{
    static const char cmd[] = "\x1b[0m";
    protocol()->write(cmd, sizeof(cmd) - 1);
}

std::string tim::vt::colorized(const std::string &s,
                               const tim::color &text_color,
                               const tim::color &bg_color)
{
    if (s.empty()
            || (text_color.empty()
                    && bg_color.empty()))
        return s;

    std::string cs;

    if (!text_color.empty())
        cs = tim::sprintf("\x1b[38;2;%u;%u;%um", text_color.r, text_color.g, text_color.b);
    if (!bg_color.empty())
        cs += sprintf("\x1b[48;2;%u;%u;%um", bg_color.r, bg_color.g, bg_color.b);

    cs += s + "\x1b[0m";

    return cs;
}

/** \param s Colorized string.
 *  \return Number of glyphs in string \a s.
 */
std::size_t tim::vt::strlen(const std::string &s)
{
    if (s.empty())
        return 0;

    /* ANSI color control sequences have the form:
     * "\x1b" "[" [0-9;]+ "m"
     * We parse them with a simple state machine.
     */

    enum class state
    {
        SearchEsc,
        ExpectBracket,
        ExpectInner,
        ExpectTrail
    };

    state st = state::SearchEsc;

    std::size_t len = 0;
    std::size_t found = 0;

    const std::size_t size = s.size();
    for (std::size_t i = 0; i < size; ++i)
    {
        const char c = s[i];

        switch (st)
        {
            case state::SearchEsc:
                len = 0;
                if (c == '\x1b')
                {
                    st = state::ExpectBracket;
                    ++len;
                }
                break;

            case state::ExpectBracket:
                if (c == '[')
                {
                    st = state::ExpectInner;
                    ++len;
                }
                else
                    st = state::SearchEsc;
                break;

            case state::ExpectInner:
                if (c >= '0'
                        && c <= '9')
                {
                    ++len;
                    st = state::ExpectTrail;
                }
                else
                    st = state::SearchEsc;
                break;

            case state::ExpectTrail:
                if (c == 'm')
                {
                    ++len;
                    found += len;
                    st = state::SearchEsc;
                }
                else if (c != ';'
                            && ((c < '0') || (c > '9')))
                    st = state::SearchEsc;
                /* 0-9, or semicolon */
                ++len;
                break;
        }
    }

    return utf8len((const utf8_int8_t *)s.c_str()) - found;
}
