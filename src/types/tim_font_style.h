#pragma once

#include "tim_flags.h"


namespace tim
{

enum class font_style
{
    Regular = 0,
    Bold    = 1 << 0,
    Italic  = 1 << 1,
    Mono    = 1 << 2
};

using font_styles = tim::flags<tim::font_style>;

}

TIM_DECL_OPERATORS_FOR_FLAGS(tim::font_styles);
