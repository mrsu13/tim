#include "tim_terminal_theme.h"


namespace tim
{

/**
 * Тёмная тема по умолчанию для VT-терминала. Подобрана как читаемая
 * палитра на тёмном фоне с акцентами warning/error/info.
 */
const terminal_theme TERMINAL_THEME_DARK
{
    .colors
    {
        { tim::terminal_color_index::text,       "#EEEEEEFF" },
        { tim::terminal_color_index::em_text,     "#EED5ADFF" },
        { tim::terminal_color_index::background, "#1A1A1AFF" },
        { tim::terminal_color_index::error,      "#FF0000B3" },
        { tim::terminal_color_index::warning,    "#F57900FF" },
        { tim::terminal_color_index::info,       "#54ABDBFF" },
        { tim::terminal_color_index::prompt,     "#6F8C1BFF" }
    },
};

}
