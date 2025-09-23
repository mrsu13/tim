#pragma once

namespace tim
{

enum class elide
{
    Left   = 0, ///< \c 0 --- The ellipsis should appear at the beginning of the text.
    Right  = 1, ///< \c 1 --- The ellipsis should appear at the end of the text.
    Middle = 2  ///< \c 2 --- The ellipsis should appear in the middle of the text.
};

}
