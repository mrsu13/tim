#pragma once

#include <filesystem>


namespace tim
{

class a_script_engine;
class line_edit;

namespace p
{

struct vt_shell
{
    static const std::string &welcome_banner();
    static const std::string &bye_banner();

    tim::a_script_engine *_engine = nullptr;
    std::unique_ptr<tim::line_edit> _ledit;
    std::filesystem::path _history_path;
};

}

}
