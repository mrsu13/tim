#pragma once

#include <memory>
#include <string>


namespace tim
{

class a_script_engine;
class vt;

namespace p
{

struct vt_shell;

}

class vt_shell
{

public:

    vt_shell(tim::vt *term, tim::a_script_engine *engine);
    virtual ~vt_shell();

    tim::vt *terminal() const;

    void new_line();
    bool write(const char *data, std::size_t size);

protected:

    virtual bool accept_command(const std::string &line, std::string &command);

private:

    std::unique_ptr<tim::p::vt_shell> _d;
};

}
