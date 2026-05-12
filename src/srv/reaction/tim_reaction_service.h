#pragma once

#include "tim_service.h"


namespace tim
{

class mqtt_client;
class sqlite_db;

namespace p
{

struct reaction_service;

}

// Сервис, обрабатывающий реакции пользователей на сообщения. Слушает топик
// react/<post-uuid>/<user-uuid> с payload-ом — целочисленным весом реакции.
// Значение weight = 0 удаляет реакцию пользователя на сообщение; ненулевое
// значение создаёт или заменяет её.
class reaction_service : public tim::service
{

public:

    reaction_service(tim::mqtt_client &mqtt, tim::sqlite_db &db);
    ~reaction_service();

private:

    std::unique_ptr<tim::p::reaction_service> _d;
};

}
