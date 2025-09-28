#pragma once

#include "tim_color.h"

#include <cassert>


namespace tim
{

class prompt_service;
class prompt_shell;
class tcl;
class telnet_server;
class vt;

namespace p
{

struct prompt_service
{
    explicit prompt_service(tim::prompt_service *q)
        : _q(q)
    {
        assert(_q);
    }

    void on_data_ready(const char *data, std::size_t size);

    tim::prompt_service *const _q;

    std::unique_ptr<tim::telnet_server> _telnet;
    std::unique_ptr<tim::vt> _terminal;
    std::unique_ptr<tim::tcl> _tcl;
    std::unique_ptr<tim::prompt_shell> _shell;
};

}

}
