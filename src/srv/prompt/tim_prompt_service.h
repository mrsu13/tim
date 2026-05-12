#pragma once

#include "tim_a_ssh_inetd_service.h"


namespace tim
{

class mqtt_client;
class sqlite_db;

namespace p
{

struct prompt_service;

}

class prompt_service : public tim::a_ssh_inetd_service
{

public:

    prompt_service(const tim::ssh_session_info &info, tim::mqtt_client &mqtt, tim::sqlite_db &db);
    ~prompt_service();

    void interrupt() noexcept override;

private:

    std::unique_ptr<tim::p::prompt_service> _d;
};

}
