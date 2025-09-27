#pragma once

#include "tim_terminal_theme.h"


namespace tim
{

class a_io_device;

namespace p
{

struct a_terminal
{
    tim::a_io_device *_io = nullptr;
    tim::terminal_theme _theme = tim::TERMINAL_THEME_DARK;
};

}

}
