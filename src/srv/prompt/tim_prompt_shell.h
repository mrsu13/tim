#pragma once

#include "tim_color.h"
#include "tim_signal.h"
#include "tim_vt_shell.h"

#include "tim_pimpl.h"


namespace tim
{

namespace p
{

struct prompt_shell;

}

/**
 * Шелл чата: расширяет vt_shell для отрисовки "плашек" сообщений и
 * перехвата ввода обычных строк как постов.
 *
 * Испускает сигнал posted с введённым текстом, если пользователь
 * нажал Enter в режиме обычного ввода (не команды /...).
 */
class prompt_shell : public tim::vt_shell
{

public:

    /**
     * Сигнал: пользователь набрал и отправил текст (не команду).
     * Аргумент — введённый текст без завершающего \\n.
     */
    tim::signal<const std::string & /* text */> posted;

    prompt_shell(tim::vt *term, tim::a_script_engine *engine);

    ~prompt_shell();

    void cloud(const std::string &title,
               const std::string &text,
               const tim::color &bg_color = tim::color::transparent(),
               const tim::color &marker_color = tim::color{});

protected:

    bool accept_command(const std::string &line, std::string &command) override;

private:

    /** PIMPL: cигналы и состояние шелла. */
    tim::pimpl<tim::p::prompt_shell> _d;
};

}
