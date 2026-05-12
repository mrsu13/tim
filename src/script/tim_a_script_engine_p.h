#pragma once

#include <string>


namespace tim
{

class a_terminal;

namespace p
{

/**
 * Внутреннее состояние tim::a_script_engine (PIMPL).
 */
struct a_script_engine
{
    /** Имя языка движка (например, "Tcl"). */
    std::string _language;
    /** Терминал, к которому привязан движок. */
    tim::a_terminal *_terminal = nullptr;
};

}

}
