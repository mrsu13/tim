#pragma once

#include <memory>


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
    ~vt_shell();

    tim::vt *terminal() const;

    bool eval(const char *data, std::size_t size);

private:

    std::unique_ptr<tim::p::vt_shell> _d;
};

}
