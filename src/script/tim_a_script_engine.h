#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>


namespace tim
{

class a_terminal;

namespace p
{

struct a_script_engine;

}

class a_script_engine
{

public:

    explicit a_script_engine(const std::string &language,
                             tim::a_terminal *term);
    virtual ~a_script_engine();

    const std::string &language() const;
    tim::a_terminal *terminal() const;

    virtual bool evaluating() const = 0;
    virtual bool eval(const std::string &program,
                      std::string *res = nullptr) = 0;
    virtual void break_eval() = 0;

    virtual const std::string &prompt() const = 0;
    virtual const std::string &error_msg() const = 0;
    virtual std::size_t error_pos() const = 0;

    virtual std::vector<std::string> complete(const std::string &prefix) const;

    virtual std::unordered_set<std::string> keywords() const = 0;
    virtual std::unordered_set<std::string> functions() const = 0;

private:

    std::unique_ptr<tim::p::a_script_engine> _d;
};

}
