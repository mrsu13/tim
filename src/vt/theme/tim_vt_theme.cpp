#include "tim_vt_theme.h"


namespace tim
{

const vt_theme VT_THEME_DARK
{
    .colors
    {
        { tim::vt_color_index::Text,       "#EEEEEEFF" },
        { tim::vt_color_index::EmText,     "#EED5ADFF" },
        { tim::vt_color_index::Background, "#1A1A1AFF" },
        { tim::vt_color_index::Error,      "#FF0000B3" },
        { tim::vt_color_index::Warning,    "#F57900FF" },
        { tim::vt_color_index::Info,       "#54ABDBFF" },
        { tim::vt_color_index::Prompt,     "#6F8C1BFF" }
    },
};

}
