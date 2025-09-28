#pragma once

#include <string>


namespace tim
{

class a_terminal;

namespace p
{

struct a_script_engine
{
    std::string _language;
    tim::a_terminal *_terminal = nullptr;
};

}

}
