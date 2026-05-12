#pragma once

#include "mongoose.h"

#include <filesystem>
#include <memory>

#ifdef TIM_OS_LINUX
#   include <signal.h>
#endif

namespace tim
{

class mqtt_client;
class post_service;
class reaction_service;
class ssh_inetd;
class user_service;
class sqlite_db;

namespace p
{

struct application
{
    static tim::application *&instance()
    {
        static tim::application *app = nullptr;
        return app;
    };

    static std::string &name()
    {
        static std::string _name;
        return _name;
    }

    static std::string &org_name()
    {
        static std::string _name;
        return _name;
    }

    static std::filesystem::path &data_dir()
    {
        static std::filesystem::path _data_dir;
        return _data_dir;
    }

#ifdef TIM_OS_LINUX

    static void signal_handler(int sig_num);

    struct sigaction _old_sig_int;
    struct sigaction _old_sig_term;
#endif
    bool _quit = false;

    struct mg_mgr _mg;
    std::unique_ptr<tim::mqtt_client> _mqtt;
    std::unique_ptr<tim::sqlite_db> _db;
    std::unique_ptr<tim::ssh_inetd> _ssh_inetd;
    std::unique_ptr<tim::post_service> _post_service;
    std::unique_ptr<tim::user_service> _user_service;
    std::unique_ptr<tim::reaction_service> _reaction_service;
};

}

}
