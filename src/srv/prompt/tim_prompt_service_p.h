#pragma once

#include "tim_user.h"

#include <cassert>
#include <filesystem>


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
    void on_post(const std::filesystem::path &topic, const char *data, std::size_t size);

    tim::prompt_service *const _q;

    std::unique_ptr<tim::telnet_server> _telnet;
    std::unique_ptr<tim::vt> _terminal;
    std::unique_ptr<tim::tcl> _tcl;
    std::unique_ptr<tim::prompt_shell> _shell;
    std::filesystem::path _topic;

    const tim::user _user
    {
        .id = "7ce5bba5-3eda-46dc-99c0-317f16bc9b3d",
        .nick = "crazy_robot",
        .icon = "🤖"
    };
};

}

}
