#pragma once

#include "tim_a_ssh_inetd_service.h"

#include <functional>


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

    // dispatch_handler — что вызывать из LIL DISPATCH-колбэка (между
    // Tcl-операторами), чтобы внешние event-loop-ы продолжали тикать,
    // пока скрипт работает. Обычно application::dispatch().
    prompt_service(const tim::ssh_session_info &info,
                   tim::mqtt_client &mqtt,
                   tim::sqlite_db &db,
                   std::function<void()> dispatch_handler);
    ~prompt_service();

    void interrupt() noexcept override;

    // Доступы к инфраструктуре сессии для Tcl-команд (через user_data
    // → prompt_service*). user_id() наследуется от a_ssh_inetd_service.
    tim::mqtt_client &mqtt() noexcept;
    tim::sqlite_db &db() noexcept;

    // UUID последнего сообщения, увиденного в этой сессии (от другого
    // пользователя или из истории). Команда /react использует его как
    // неявную цель.
    const tim::uuid &last_seen_post() const noexcept;

private:

    std::unique_ptr<tim::p::prompt_service> _d;
};

}
