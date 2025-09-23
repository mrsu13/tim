#pragma once

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

    void exec();
    void quit();

    mg_mgr *mongoose() const;

private:

    std::unique_ptr<tim::p::app> _d;
};
