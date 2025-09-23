#pragma once

#include "mongoose.h"

#include <memory>


namespace tim
{

class inetd;

namespace p
{

struct app
{
    static std::string &name()
    {
        static std::string _name;
        return _name;
    }

    static std::string &org_name()
    {
        static std::string _name;
        return _name;
    }

    bool _quit = false;

    struct mg_mgr _mg;
    std::unique_ptr<tim::inetd> _tcl_shell_inetd;
};

}

}
