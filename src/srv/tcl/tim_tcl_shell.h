#pragma once

#include "tim_a_telnet_service.h"


namespace tim
{

namespace p
{

struct tcl_shell;

}

class tcl_shell : public tim::a_telnet_service
{

public:

    explicit tcl_shell(mg_connection *c);
    ~tcl_shell();

    bool process_data(const char *data, std::size_t size) override;

private:

    std::unique_ptr<tim::p::tcl_shell> _d;
};

}
