#pragma once

#include <filesystem>
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

    // Каталог, в котором живут БД, SSH host-key, история шелла и TLS-сертификаты.
    // Выставляется в конструкторе application из tim::settings::load_or_create();
    // после этого доступен как глобальная константа времени жизни приложения.
    static const std::filesystem::path &data_dir();

    void dispatch();
    void exec();
    void quit();

private:

    std::unique_ptr<tim::p::application> _d;
};

}
