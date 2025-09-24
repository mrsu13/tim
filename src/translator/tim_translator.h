#pragma once

#include "tim_translation.h"

#include <memory>


namespace tim
{

namespace p
{

struct translator;

}

class translator
{

public:

    ~translator();

    static const tim::translator &instance();

    static const char *translate(const tim::translations &translations,
                                 const char *file_path, int line);

private:

    translator();

    std::unique_ptr<tim::p::translator> _d;
};

}

#define TIM_TR(...) tim::translator::translate({ __VA_ARGS__ }, __FILE__, __LINE__)
