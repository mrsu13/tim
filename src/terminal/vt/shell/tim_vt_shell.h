#pragma once

#include <string>

#include "tim_pimpl.h"


namespace tim
{

class a_script_engine;
class vt;

namespace p
{

struct vt_shell;

}

/**
 * Минимальный интерактивный шелл поверх VT-терминала.
 *
 * Печатает приглашение, читает строку через line_edit, и при Enter
 * передаёт её в a_script_engine на выполнение. Наследники
 * (prompt_shell) могут перехватывать ввод не-команд через
 * accept_command().
 */
class vt_shell
{

public:

    vt_shell(tim::vt *term, tim::a_script_engine *engine);

    virtual ~vt_shell();

    tim::vt *terminal() const;

    void new_line();

    bool write(const char *data, std::size_t size);

    void hide_input();

    void show_input();

protected:

    virtual bool accept_command(const std::string &line, std::string &command);

private:

    /** PIMPL: line_edit, путь к истории, ссылка на engine. */
    tim::pimpl<tim::p::vt_shell> _d;
};

}
