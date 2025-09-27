#include "tim_terminal_theme.h"


namespace tim
{

const terminal_theme TERMINAL_THEME_DARK
{
    .colors
    {
        { tim::terminal_color_index::Text,       "#EEEEEEFF" },
        { tim::terminal_color_index::EmText,     "#EED5ADFF" },
        { tim::terminal_color_index::Background, "#1A1A1AFF" },
        { tim::terminal_color_index::Error,      "#FF0000B3" },
        { tim::terminal_color_index::Warning,    "#F57900FF" },
        { tim::terminal_color_index::Info,       "#54ABDBFF" },
        { tim::terminal_color_index::Prompt,     "#6F8C1BFF" }
    },
};

}
