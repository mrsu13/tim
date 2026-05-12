#pragma once

#include "tim_terminal_theme.h"


namespace tim
{

class a_protocol;

namespace p
{

/**
 * Внутреннее состояние tim::a_terminal (PIMPL).
 */
struct a_terminal
{
    /** Указатель на протокол. */
    tim::a_protocol *_proto = nullptr;
    /** Текущая тема (по умолчанию тёмная). */
    tim::terminal_theme _theme = tim::TERMINAL_THEME_DARK;
};

}

}
