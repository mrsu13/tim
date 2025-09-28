#pragma once

#include "tim_vt_shell.h"


namespace tim
{

namespace p
{

struct prompt_shell;

}

class prompt_shell : public tim::vt_shell
{

public:

    prompt_shell(tim::vt *term, tim::a_script_engine *engine);
    ~prompt_shell();

protected:

    bool accept_command(const std::string &line, std::string &command) const override;

private:

    std::unique_ptr<tim::p::prompt_shell> _d;
};

}
