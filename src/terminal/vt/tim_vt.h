#pragma once

#include "tim_a_terminal.h"

#include <string>

#include "tim_pimpl.h"


namespace tim
{

class a_terminal_protocol;

namespace p
{

struct vt;

}

/**
 * VT100/xterm-эмулятор: реализация a_terminal через ANSI escape-sequences.
 *
 * Управление цветами, перемещением курсора и очисткой экрана выполняется
 * через стандартные ESC-последовательности; терминал клиента сам их
 * интерпретирует.
 */
class vt : public tim::a_terminal
{

public:

    explicit vt(tim::a_terminal_protocol *proto);

    ~vt();

    std::size_t rows() const override;

    std::size_t cols() const override;

    void clear() override;

    std::size_t color_count() const override;

    tim::color color(std::size_t index) const override;

    void set_color(const tim::color &c) override;

    void set_color(std::size_t index) override;

    void set_default_color() override;

    void set_bg_color(const tim::color &c) override;

    void set_bg_color(std::size_t index) override;

    void reverse_colors() override;

    void reset_colors() override;

    static std::string colorized(const std::string &s,
                                 const tim::color &text_color,
                                 const tim::color &bg_color = tim::color{});

    static std::size_t strlen(const std::string &s);

private:

    /** PIMPL: цвета, буферы, состояние ANSI-state machine. */
    tim::pimpl<tim::p::vt> _d;
};

}
