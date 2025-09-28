#include "tim_a_script_engine.h"

#include "tim_a_script_engine_p.h"

#include <cassert>


// Public

tim::a_script_engine::a_script_engine(const std::string &language,
                                      tim::a_terminal *term)
    : _d(new tim::p::a_script_engine())
{
    assert(!language.empty() && "Script language name must not be empty.");
    assert(term);

    _d->_language = language;
    _d->_terminal = term;
}

tim::a_script_engine::~a_script_engine() = default;

const std::string &tim::a_script_engine::language() const
{
    return _d->_language;
}

tim::a_terminal *tim::a_script_engine::terminal() const
{
    return _d->_terminal;
}

std::vector<std::string> tim::a_script_engine::complete(const std::string &prefix) const
{
    (void) prefix;

    return {};
}
