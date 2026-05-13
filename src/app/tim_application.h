#pragma once

#include <filesystem>
#include <string>

#include "tim_pimpl.h"


namespace tim
{

namespace p
{

struct application;

}

/**
 * Корневой класс приложения TIM.
 *
 * Загружает настройки, поднимает подсистемы (MQTT-клиент, SQLite,
 * SSH inetd, серверные сервисы post/user/reaction) и крутит
 * event-loop через exec(). Конструируется один раз в main().
 */
class application
{

public:

    explicit application(const std::string &config_path = {});

    ~application();

    static const std::string &name();

    static void set_name(const std::string &name);

    static const std::string &org_name();

    static void set_org_name(const std::string &name);

    static const std::filesystem::path &data_dir();

    void dispatch();

    void exec();

    void quit();

private:

    /** PIMPL: всё внутреннее состояние (signals, mongoose, подсистемы). */
    tim::pimpl<tim::p::application> _d;
};

}
