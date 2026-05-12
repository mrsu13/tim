#pragma once

#include <cassert>


namespace tim
{

class a_terminal_protocol;
class vt;

namespace p
{

/**
 * Внутреннее состояние tim::vt (PIMPL).
 */
struct vt
{
    /**
     * \param q Указатель на внешний tim::vt; используется методами,
     *          которым нужен доступ к публичному API родителя.
     */
    explicit vt(tim::vt *q)
        : _q(q)
    {
        assert(_q);
    }

    /** Внешний tim::vt (владелец). */
    tim::vt *const _q;

    /** Терминальный протокол: источник rows/cols/term_name и приёмник байт. */
    tim::a_terminal_protocol *_term_proto = nullptr;
};

}

}
