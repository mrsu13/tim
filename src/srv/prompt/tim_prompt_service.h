#pragma once

#include "tim_a_ssh_inetd_service.h"

#include <functional>

#include "tim_pimpl.h"


namespace tim
{

class mqtt_client;
class sqlite_db;

namespace p
{

struct prompt_service;

}

/**
 * Чат-сессия одного пользователя.
 *
 * Конструируется фабрикой ssh_inetd при подключении клиента; владеет
 * SSH-терминальным протоколом, VT-эмулятором, Tcl-интерпретатором,
 * prompt_shell, profile_cache и набором MQTT-подписок (POST_FILTER,
 * REACT_EVENT_FILTER, user/subscribe/<self>, user/unsubscribe/<self>,
 * session/notice/<self>).
 */
class prompt_service : public tim::a_ssh_inetd_service
{

public:

    prompt_service(const tim::ssh_session_info &info,
                   tim::mqtt_client &mqtt,
                   tim::sqlite_db &db,
                   std::function<void()> dispatch_handler);

    ~prompt_service();

    void interrupt() noexcept override;

    tim::mqtt_client &mqtt() noexcept;

    tim::sqlite_db &db() noexcept;

    const tim::uuid &last_seen_post() const noexcept;

private:

    /** PIMPL: терминал, Tcl, подписки, кэши. */
    tim::pimpl<tim::p::prompt_service> _d;
};

}
