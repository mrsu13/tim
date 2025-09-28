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

    void cloud(const std::string &text,
               const tim::color &bg_color = tim::color::transparent());

    tim::prompt_shell *const _q;
};

}

}
