#pragma once

#include "tim_flags.h"


namespace tim
{

/**
 * Атрибуты начертания шрифта. Битовая маска: можно сочетать через |.
 */
enum class font_style
{
    regular = 0,         ///< Обычное начертание.
    bold    = 1 << 0,    ///< Полужирное.
    italic  = 1 << 1,    ///< Курсив.
    mono    = 1 << 2     ///< Моноширинный шрифт.
};

/**
 * Битовая маска font_style — позволяет комбинировать атрибуты.
 */
using font_styles = tim::flags<tim::font_style>;

}

TIM_DECL_OPERATORS_FOR_FLAGS(tim::font_styles);
