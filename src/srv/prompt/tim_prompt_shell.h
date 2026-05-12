#pragma once

#include "tim_color.h"
#include "tim_signal.h"
#include "tim_vt_shell.h"


namespace tim
{

namespace p
{

struct prompt_shell;

}

class prompt_shell : public tim::vt_shell
{

public:

    tim::signal<const std::string & /* text */> posted;

    prompt_shell(tim::vt *term, tim::a_script_engine *engine);
    ~prompt_shell();

    // Если задан marker_color, перед заголовком в полосе плашки рисуется
    // маркер ('★') этим цветом; сам заголовок отображается обычным
    // авто-контрастным цветом текста. Используется для пометки сообщений
    // от пользователей, на которых вы подписаны.
    void cloud(const std::string &title,
               const std::string &text,
               const tim::color &bg_color = tim::color::transparent(),
               const tim::color &marker_color = tim::color{});

protected:

    bool accept_command(const std::string &line, std::string &command) override;

private:

    std::unique_ptr<tim::p::prompt_shell> _d;
};

}
