#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef enum tim_vt_color
{
    TimVtNoColor = -1,
    TimVtBlack   = 0,
    TimVtRed     = 1,
    TimVtGreen   = 2,
    TimVtYellow  = 3,
    TimVtBlue    = 4,
    TimVtMagenta = 5,
    TimVtCyan    = 6,
    TimVtWhite   = 7
} tim_vt_color_t;

char *tim_vt_colorize(const char *s, int text_color, int bg_color);
size_t tim_vt_strlen(const char *s);
