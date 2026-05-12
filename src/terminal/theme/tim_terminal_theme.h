#pragma once

#include "tim_terminal_color_theme.h"


namespace tim
{

/**
 * Терминальная тема. Сейчас содержит только палитру цветов; в будущем
 * может расшириться шрифтовыми атрибутами, рамками и т.п.
 */
struct terminal_theme
{
    /** Палитра цветов по семантическим индексам. */
    tim::terminal_color_theme colors;
};

/** Готовая тёмная тема (определена в tim_terminal_theme.cpp). */
extern const terminal_theme TERMINAL_THEME_DARK;

}
