#pragma once

#include <memory>
#include <string>


struct mg_mgr;

namespace tim
{

class mqtt_client;

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

    void exec();
    void quit();

    mg_mgr *mongoose() const;
    tim::mqtt_client *mqtt() const;

private:

    std::unique_ptr<tim::p::application> _d;
};

tim::application *app();

}
