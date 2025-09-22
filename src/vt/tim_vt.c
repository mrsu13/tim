#include "tim_vt.h"

#include "utf8.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


char *tim_vt_colorize(const char *s, int text_color, int bg_color)
{
    assert(s);

    if (!*s
            || (text_color < 0
                    && bg_color < 0))
        return strdup(s);

    const size_t len = strlen(s);
    char *cs = (char *)calloc(1, len
                                    + 8 // Maximum size of text color.
                                    + 8 // Maximum size of bg color.
                                    + 4 // Back to normal colors.
                                    + 1); // \0
    assert(cs && "Failed to allocate memory for color string.");

    int i = 0;
    if (text_color >= 0)
    {
        const uint8_t bold = (uint8_t)(text_color & 0xFF) >> 4;
        if (bold)
            i = sprintf(cs, "\x1b[3%u;1m", bold);
        else
            i = sprintf(cs, "\x1b[3%dm", text_color);
    }
    if (bg_color >= 0)
        i += sprintf(cs + i, "\x1b[4%dm", bg_color);

    memcpy(cs + i, s, len);
    i += len;
    memcpy(cs + i, "\x1b[0m", 4);

    return cs;
}

/**
 * \param s Colorized string.
 * \return Number of characters in string \a s.
 */
size_t tim_vt_strlen(const char *s)
{
    assert(s);

    if (!*s)
        return 0;

    /* ANSI color control sequences have the form:
     * "\x1b" "[" [0-9;]+ "m"
     * We parse them with a simple state machine.
     */

    typedef enum state
    {
        SearchEsc,
        ExpectBracket,
        ExpectInner,
        ExpectTrail
    } state_t;

    state_t state = SearchEsc;

    size_t len = 0;
    size_t found = 0;

    const size_t size = strlen(s);
    for (size_t i = 0; i < size; ++i)
    {
        const char c = s[i];

        switch (state)
        {
            case SearchEsc:
                len = 0;
                if (c == '\x1b')
                {
                    state = ExpectBracket;
                    ++len;
                }
                break;

            case ExpectBracket:
                if (c == '[')
                {
                    state = ExpectInner;
                    ++len;
                }
                else
                    state = SearchEsc;
                break;

            case ExpectInner:
                if (c >= '0'
                        && c <= '9')
                {
                    ++len;
                    state = ExpectTrail;
                }
                else
                    state = SearchEsc;
                break;

            case ExpectTrail:
                if (c == 'm')
                {
                    ++len;
                    found += len;
                    state = SearchEsc;
                }
                else if (c != ';'
                            && ((c < '0') || (c > '9')))
                    state = SearchEsc;
                /* 0-9, or semicolon */
                ++len;
                break;
        }
    }

    return utf8len(s) - found;
}
