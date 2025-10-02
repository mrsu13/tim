#pragma once

#include "tim_color.h"

#include <cassert>


namespace tim
{

class prompt_shell;

namespace p
{

struct prompt_shell
{
    explicit prompt_shell(tim::prompt_shell *q)
        : _q(q)
    {
        assert(_q);
    }

    tim::prompt_shell *const _q;
};

}

}
