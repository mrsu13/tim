#include "tim_tcl_shell.h"

#include "tim_tcl_shell_p.h"

#include "tim_config.h"
#include "tim_line_edit.h"
#include "tim_tcl.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include "fort.h"

#include <cassert>


// Public

tim::tcl_shell::tcl_shell(mg_connection *c)
    : tim::a_telnet_service("tcl_shell", c)
    , _d(new tim::p::tcl_shell())
{
    _d->_tcl.reset(new tim::tcl(this));
    _d->_ledit.reset(new tim::line_edit(this));
    _d->_ledit->set_prompt(colorized(_d->_tcl->prompt(), theme().colors.at(tim::vt_color_index::Prompt)));

    write_str(tim::p::tcl_shell::welcome_banner());

    _d->_ledit->new_line();
}

tim::tcl_shell::~tcl_shell() = default;

bool tim::tcl_shell::process_data(const char *data, std::size_t size)
{
    assert(data);

    if (_d->_tcl->evaluating())
        return true;

    switch (_d->_ledit->get_line(data, size))
    {
        case tim::line_edit::status::Finished:
        {
            if (!_d->_ledit->empty())
            {
                write("\n", 1);
                const std::string &line = _d->_ledit->line();
                _d->_ledit->history_save(_d->_history_path);
                std::string command;
                if (get_command(line, command)
                        && !command.empty())
                {
                    std::string res;
                    if (_d->_tcl->eval(command, &res))
                    {
                        if (!res.empty())
                            write(res.c_str(), res.size());
                    }
                    else
                    {
                        const std::size_t pos = _d->_tcl->error_pos();

                        set_color(theme().colors.at(tim::vt_color_index::Error));
                        this->printf(TIM_TR("Error: %s\n%s\n"_en,
                                            "Ошибка. %s\n%s\n"_ru),
                                     _d->_tcl->error_msg().c_str(),
                                     command.c_str());
                        if (pos)
                        {
                            static const char hr[] = "─";
                            for (std::size_t i = 0; i < pos - 1; ++i)
                                write(hr, sizeof(hr) - 1);
                        }
                        {
                            static const char arrow[] = "^";
                            write(arrow, sizeof(arrow) - 1);
                        }
                        reset_colors();
                    }
                }
                _d->_ledit->new_line();
            }
            break;
        }
        case tim::line_edit::status::Continue:
            break;

        case tim::line_edit::status::Exit:
            write_str(tim::p::tcl_shell::bye_banner());
            return false;

        case tim::line_edit::status::Error:
            _d->_ledit->new_line();
            break;
    }

    return true;
}


// Protected

bool tim::tcl_shell::get_command(const std::string &line, std::string &command)
{
    command = line;
    return true;
}


// Private

const std::string &tim::p::tcl_shell::welcome_banner()
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

const std::string &tim::p::tcl_shell::bye_banner()
{
    static const std::string banner = "Bye!\n";
    return banner;
}
