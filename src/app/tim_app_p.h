#pragma once

#include "mongoose.h"

#include <memory>


namespace tim
{

class inetd;

namespace p
{

struct tim_app
{
    bool _quit = false;

    struct mg_mgr _mg;
    std::unique_ptr<tim::inetd> *tcl_shell_inetd;
};

}

}
