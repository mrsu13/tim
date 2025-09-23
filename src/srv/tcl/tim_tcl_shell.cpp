#include "tim_tcl_shell.h"

#include "tim_tcl_shell_p.h"

#include "tim_config.h"
#include "tim_line_edit.h"
#include "tim_tcl.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include <cassert>


// Public

tim::tcl_shell::tcl_shell(mg_connection *c)
    : tim::a_telnet_service("tcl_shell", c)
    , _d(new tim::p::tcl_shell())
{
    _d->_tcl.reset(new tim::tcl(this));
    _d->_ledit.reset(new tim::line_edit(this));
    _d->_ledit->set_prompt(_d->_tcl->prompt());

    write_str(tim::p::tcl_shell::welcome_banner());

    _d->_ledit->new_line();
}

tim::tcl_shell::~tcl_shell()
{
    write_str(tim::p::tcl_shell::bye_banner());
}

bool tim::tcl_shell::process_data(const char *data, std::size_t size)
{
    assert(data);

    if (_d->_tcl->evaluating())
        return true;

    if (_d->_new_line)
    {
        _d->_new_line = false;
        _d->_ledit->new_line();
    }

    switch (_d->_ledit->get_line(data, size))
    {
        case tim::line_edit::status::Finished:
        {
            _d->_new_line = true;
            if (!_d->_ledit->empty())
            {
                write("\r\n", 2);
                const std::string &line = _d->_ledit->line();
                _d->_ledit->history_save(_d->_history_path);

                std::string res;
                if (_d->_tcl->eval(line, &res))
                {
                    if (!res.empty())
                        write(res.c_str(), res.size());
                }
                else
                {
                    const std::size_t pos = _d->_tcl->error_pos();

                    //set_color(theme().colors.at(tim::vt_color_index::Error));
                    this->printf(TIM_TR("Error: %s\n%s\n"_en,
                                        "Ошибка. %s\n%s\n"_ru),
                                 _d->_tcl->error_msg().c_str(),
                                 line.c_str());
                    if (pos)
                    {
                        static const char hr[] = "─";
                        for (std::size_t i = 0; i < pos - 1; ++i)
                            write(hr, sizeof(hr) - 1);
                    }
                    {
                        static const char arrow[] = "┘";
                        write(arrow, sizeof(arrow) - 1);
                    }
                    //reset_colors();
                }
            }
            break;
        }
        case tim::line_edit::status::Continue:
            break;

        case tim::line_edit::status::Exit:
            return false;

        case tim::line_edit::status::Error:
            _d->_new_line = true;
            break;
    }

    return true;
}

// Private

const char *tim::p::tcl_shell::welcome_banner()
{
    return "Welcome to TIM!\n";
}

const char *tim::p::tcl_shell::bye_banner()
{
    return "Bye!\n";
}
