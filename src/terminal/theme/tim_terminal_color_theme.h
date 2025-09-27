#pragma once

#include "tim_color.h"

#include <unordered_map>


namespace tim
{
    
enum class terminal_color_index
{
    Text, ///< Regular text.
    EmText, ///< Emphasized text.
    Background, ///< Regular background.

    Error, ///< Error message.
    Warning, ///< Warning message.
    Info, ///< Informational message.

    Prompt, ///< Prompt color.

    Count ///< Number of colors.
};

using terminal_color_theme = std::unordered_map<tim::terminal_color_index, tim::color>;

}
