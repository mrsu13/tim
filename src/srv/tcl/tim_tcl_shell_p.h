#pragma once

#include <filesystem>


namespace tim
{

class tcl;
class line_edit;

namespace p
{

struct tcl_shell
{
    static const char *welcome_banner();
    static const char *bye_banner();

    std::unique_ptr<tim::tcl> _tcl;
    std::unique_ptr<tim::line_edit> _ledit;
    std::filesystem::path _history_path = "/tmp/tim.txt";
};

}

}
