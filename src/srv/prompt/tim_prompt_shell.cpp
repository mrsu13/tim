#include "tim_prompt_shell.h"

#include "tim_prompt_shell_p.h"

#include "tim_a_protocol.h"
#include "tim_config.h"
#include "tim_string_tools.h"
#include "tim_vt.h"

#include "fort.h"


static const std::string LOREM_IPSUM =
"Lorem ipsum dolor sit amet, consectetur adipiscing elit, \
sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. \
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris \
nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in \
reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla \
pariatur. Excepteur sint occaecat cupidatat non proident, \
sunt in culpa qui officia deserunt mollit anim id est laborum.";

// Public

tim::prompt_shell::prompt_shell(tim::vt *term, tim::a_script_engine *engine)
    : tim::vt_shell(term, engine)
    , posted()
    , _d(new tim::p::prompt_shell(this))
{
}

tim::prompt_shell::~prompt_shell() = default;

void tim::prompt_shell::cloud(const std::string &text, const tim::color &bg_color)
{
    const std::string t = tim::trim(text);
    if (t.empty())
        return;

    if (text.at(0) == '\n')
        terminal()->protocol()->write("\n", 1);

    terminal()->set_bg_color(bg_color);
    terminal()->set_color(bg_color.text_color());

    ft_table_t *table = ft_create_table();
    ft_u8write_ln(table,
                  tim::aligned(t, tim::text_align::Justify,
                               t.size() > terminal()->cols() * 2
                                   ? terminal()->cols() / 2
                                   : terminal()->cols() / 3).c_str());
    const std::string_view table_text((const char *)ft_to_u8string(table));
    terminal()->protocol()->write(table_text.data(), table_text.size() - 1); // No \n at the end.
    ft_destroy_table(table);

    terminal()->reset_colors();
}


// Protected

bool tim::prompt_shell::accept_command(const std::string &line, std::string &command)
{
    if (line.empty())
        return false;

    if (line.size() < 2
            || line[0] != tim::COMMAND_PREFIX)
    {
        const std::string &l = line == "lorem"
                                    ? LOREM_IPSUM
                                    : line;
        cloud(l);
        posted(l);
        return false;
    }

    command = line.substr(1);

    return true;
}
