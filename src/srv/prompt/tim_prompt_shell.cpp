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
    , _d(new tim::p::prompt_shell(this))
{
}

tim::prompt_shell::~prompt_shell() = default;


// Protected

bool tim::prompt_shell::accept_command(const std::string &line, std::string &command) const
{
    if (line.empty())
        return false;

    if (line.size() < 2
            || line[0] != tim::COMMAND_PREFIX)
    {
        _d->cloud(line == "lorem"
                    ? LOREM_IPSUM
                    : line);
        return false;
    }

    command = line.substr(1);

    return true;
}


// Private

void tim::p::prompt_shell::cloud(const std::string &text, const tim::color &bg_color)
{
    _q->terminal()->set_bg_color(bg_color);
    _q->terminal()->set_color(bg_color.text_color());

    ft_table_t *table = ft_create_table();
    ft_u8write_ln(table,
                  tim::aligned(text, tim::text_align::Justify,
                               text.size() > _q->terminal()->cols() * 2
                                   ? _q->terminal()->cols() / 2
                                   : _q->terminal()->cols() / 3).c_str());
    const std::string_view table_text((const char *)ft_to_u8string(table));
    _q->terminal()->protocol()->write(table_text.data(), table_text.size() - 1); // No \n at the end.
    ft_destroy_table(table);

    _q->terminal()->reset_colors();
}
