#pragma once

#include <memory>
#include <string>


namespace tim
{

namespace p
{

struct application;

}

class application
{

public:

    application(int argc, char **argv);
    ~application();

    static const std::string &name();
    static void set_name(const std::string &name);

    static const std::string &org_name();
    static void set_org_name(const std::string &name);

    void dispatch();
    void exec();
    void quit();

private:

    std::unique_ptr<tim::p::application> _d;
};

}
