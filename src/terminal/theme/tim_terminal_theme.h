#pragma once

#include "tim_terminal_color_theme.h"


namespace tim
{

struct terminal_theme
{
    tim::terminal_color_theme colors;
};

extern const terminal_theme TERMINAL_THEME_DARK;

}
