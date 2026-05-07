#pragma once

#include "tim_service.h"


namespace tim
{

class mqtt_client;
class sqlite_db;

namespace p
{

struct post_service;

}

class post_service : public tim::service
{

public:

    post_service(tim::mqtt_client &mqtt, tim::sqlite_db &db);
    ~post_service();

private:

    std::unique_ptr<tim::p::post_service> _d;
};

}
