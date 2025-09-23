#pragma once

#include <cstddef>


namespace tim
{

namespace p
{

struct tcl;

}

class tcl
{

public:

    tcl();
    virtual ~tcl();

    bool evaluating() const;
    bool eval(const std::string &program, std::string *res = nullptr, void *user_data = nullptr);
    void break_eval();

    const std::string &prompt() const;
    const char *error_msg() const;
    std::size_t error_pos() const;

protected:

    virtual void write(const char *msg);
    virtual void dispatch();

private:

    std::unique_ptr<tim::p::tcl> _d;
};

}
