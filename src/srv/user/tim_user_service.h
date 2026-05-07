#pragma once

#include "tim_service.h"


namespace tim
{

class mqtt_client;
class sqlite_db;

namespace p
{

struct user_service;

}

class user_service : public tim::service
{

public:

    user_service(tim::mqtt_client &mqtt, tim::sqlite_db &db);
    ~user_service();

private:

    std::unique_ptr<tim::p::user_service> _d;
};

}
