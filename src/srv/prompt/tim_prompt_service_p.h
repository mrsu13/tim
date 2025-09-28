#pragma once

#include "tim_color.h"

#include <cassert>


namespace tim
{

class prompt_service;
class tcl;
class telnet;
class vt;
class vt_shell;

namespace p
{

struct prompt_service
{
    explicit prompt_service(tim::prompt_service *q)
        : _q(q)
    {
        assert(_q);
    }

    void on_ready_read(const char *data, std::size_t size);

    void cloud(const std::string &text,
               const tim::color &bg_color = tim::color::transparent());

    tim::prompt_service *const _q;

    std::unique_ptr<tim::telnet> _telnet;
    std::unique_ptr<tim::vt> _terminal;
    std::unique_ptr<tim::tcl> _tcl;
    std::unique_ptr<tim::vt_shell> _shell;
};

}

}
