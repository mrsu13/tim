#pragma once

#include <cstddef>
#include <memory>
#include <string>


namespace tim
{

namespace p
{

struct tcl;

}

class a_telnet_service;

class tcl
{

public:

    explicit tcl(tim::a_telnet_service *telnet);
    virtual ~tcl();

    bool evaluating() const;
    bool eval(const std::string &program, std::string *res = nullptr);
    void break_eval();

    const std::string &prompt() const;
    const std::string &error_msg() const;
    std::size_t error_pos() const;

private:

    std::unique_ptr<tim::p::tcl> _d;
};

}
