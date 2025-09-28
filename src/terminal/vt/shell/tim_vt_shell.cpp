#include "tim_vt_shell.h"

#include "tim_vt_shell_p.h"

#include "tim_a_protocol.h"
#include "tim_a_script_engine.h"
#include "tim_config.h"
#include "tim_file_tools.h"
#include "tim_line_edit.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_vt.h"

#include "fort.h"

#include <cassert>


// Public

tim::vt_shell::vt_shell(tim::vt *term, tim::a_script_engine *engine)
    : _d(new tim::p::vt_shell())
{
    assert(term);
    assert(engine);

    _d->_history_path
        = tim::complete_path(tim::standard_location(tim::filesystem_location::AppData)
                                / tim::HISTORY_FNAME,
                             tim::create_path::Base);

    _d->_engine = engine;
    _d->_ledit.reset(new tim::line_edit(term));
    _d->_ledit->set_prompt(tim::vt::colorized(_d->_engine->prompt(),
                                              term->theme().colors.at(tim::terminal_color_index::Prompt)));
    _d->_ledit->history_load(_d->_history_path);

    term->protocol()->write_str(tim::p::vt_shell::welcome_banner());

    _d->_ledit->new_line();
}

tim::vt_shell::~vt_shell() = default;

tim::vt *tim::vt_shell::terminal() const
{
    return _d->_ledit->terminal();
}

bool tim::vt_shell::write(const char *data, std::size_t size)
{
    assert(data);

    if (!size
            || _d->_engine->evaluating())
        return true;

    switch (_d->_ledit->get_line(data, size))
    {
        case tim::line_edit::status::Finished:
        {
            if (!_d->_ledit->empty())
            {
                _d->_ledit->terminal()->protocol()->write("\n", 1);
                const std::string &line = _d->_ledit->line();
                _d->_ledit->history_save(_d->_history_path);
                std::string command;
                if (accept_command(line, command)
                        && !command.empty())
                {
                    std::string res;
                    if (_d->_engine->eval(command, &res))
                    {
                        if (!res.empty())
                            _d->_ledit->terminal()->protocol()->write(res.c_str(), res.size());
                    }
                    else
                    {
                        const std::size_t pos = _d->_engine->error_pos();

                        _d->_ledit->terminal()->set_color(
                            _d->_ledit->terminal()->theme().colors.at(tim::terminal_color_index::Error));
                        _d->_ledit->terminal()
                            ->printf(TIM_TR("Error: %s\n%s\n"_en,
                                            "Ошибка. %s\n%s\n"_ru),
                                     _d->_engine->error_msg().c_str(),
                                     command.c_str());
                        if (pos)
                        {
                            static const char hr[] = "─";
                            for (std::size_t i = 0; i < pos - 1; ++i)
                                _d->_ledit->terminal()->protocol()->write(hr, sizeof(hr) - 1);
                        }
                        {
                            static const char arrow[] = "^";
                            _d->_ledit->terminal()->protocol()->write(arrow, sizeof(arrow) - 1);
                        }
                        _d->_ledit->terminal()->reset_colors();
                    }
                }
                _d->_ledit->new_line();
            }
            break;
        }
        case tim::line_edit::status::Continue:
            break;

        case tim::line_edit::status::Exit:
            _d->_ledit->terminal()->protocol()->write_str(tim::p::vt_shell::bye_banner());
            return false;

        case tim::line_edit::status::Error:
            _d->_ledit->new_line();
            break;
    }

    return true;
}


// Protected

bool tim::vt_shell::accept_command(const std::string &line, std::string &command) const
{
    command = line;
    return true;
}


// Private

const std::string &tim::p::vt_shell::welcome_banner()
{
    static std::string banner;
    if (banner.empty())
    {
        ft_table_t *table = ft_create_table();
        ft_set_cell_prop(table, FT_ANY_ROW, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
        ft_write_ln(table, "Welcome to TIM!");

        banner = ft_to_string(table);
        ft_destroy_table(table);
    }
    return banner;
}

const std::string &tim::p::vt_shell::bye_banner()
{
    static const std::string banner = "Bye!\n";
    return banner;
}
