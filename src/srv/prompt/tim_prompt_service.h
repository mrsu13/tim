#pragma once

#include "tim_tcl_shell.h"


namespace tim
{
    
namespace p
{

struct prompt_service;

}

class prompt_service : public tim::tcl_shell
{

public:

    explicit prompt_service(mg_connection *c);
    ~prompt_service();

protected:

    bool get_command(const std::string &line, std::string &command) override;

private:

    std::unique_ptr<tim::p::prompt_service> _d;
};

}
