#pragma once

#include "tim_color.h"
#include "tim_signal.h"
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

    tim::signal<const std::string & /* text */> posted;

    prompt_shell(tim::vt *term, tim::a_script_engine *engine);
    ~prompt_shell();

    void cloud(const std::string &text,
               const tim::color &bg_color = tim::color::transparent());

protected:

    bool accept_command(const std::string &line, std::string &command) override;

private:

    std::unique_ptr<tim::p::prompt_shell> _d;
};

}
