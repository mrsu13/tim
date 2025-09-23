#pragma once

#include <memory>
#include <string>


struct mg_mgr;

namespace tim
{

namespace p
{

struct app;

}

class app
{

public:

    app(int argc, char **argv);
    ~app();

    static const std::string &name();
    static void set_name(const std::string &name);

    static const std::string &org_name();
    static void set_org_name(const std::string &name);

    void exec();
    void quit();

    mg_mgr *mongoose() const;

private:

    std::unique_ptr<tim::p::app> _d;
};

}
