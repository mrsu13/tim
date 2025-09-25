#pragma once

#include <cassert>


namespace tim
{

class prompt_service;
struct color;

namespace p
{

struct prompt_service
{
    explicit prompt_service(tim::prompt_service *q)
        : _q(q)
    {
        assert(_q);
    }

    void cloud(const std::string &text, const tim::color &bg_color = tim::color::transparent());

    tim::prompt_service *const _q;
};

}

}
