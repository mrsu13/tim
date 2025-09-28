#pragma once

#include "tim_terminal_theme.h"


namespace tim
{

class a_protocol;

namespace p
{

struct a_terminal
{
    tim::a_protocol *_proto = nullptr;
    tim::terminal_theme _theme = tim::TERMINAL_THEME_DARK;
};

}

}
