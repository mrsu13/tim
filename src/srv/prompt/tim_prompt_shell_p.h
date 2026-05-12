#pragma once

#include "tim_color.h"

#include <cassert>


namespace tim
{

class prompt_shell;

namespace p
{

/**
 * Внутреннее состояние tim::prompt_shell (PIMPL). Сейчас минимально:
 * хранит обратный указатель для редких операций, требующих публичного API.
 */
struct prompt_shell
{
    /**
     * \param q Указатель на внешний prompt_shell.
     */
    explicit prompt_shell(tim::prompt_shell *q)
        : _q(q)
    {
        assert(_q);
    }

    /** Внешний prompt_shell (владелец). */
    tim::prompt_shell *const _q;
};

}

}
