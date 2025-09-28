#pragma once

#include "tim_a_terminal.h"

#include <string>


namespace tim
{

namespace p
{

struct vt;

}

class vt : public tim::a_terminal
{

public:

    explicit vt(tim::a_protocol *proto);
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

    std::unique_ptr<tim::p::vt> _d;
};

}
