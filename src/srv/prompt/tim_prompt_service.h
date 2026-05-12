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

    /**
     * Конструктор. Поднимает терминал, Tcl и подписки.
     *
     * \param info Параметры SSH-сессии (terminal, user_id, размеры).
     * \param mqtt MQTT-клиент приложения; должен жить дольше сессии.
     * \param db Подключение к БД; должно жить дольше сессии.
     * \param dispatch_handler Колбэк, который Tcl-интерпретатор будет
     *                         вызывать между операторами для кручения
     *                         внешних event-loop-ов. Обычно
     *                         application::dispatch().
     */
    prompt_service(const tim::ssh_session_info &info,
                   tim::mqtt_client &mqtt,
                   tim::sqlite_db &db,
                   std::function<void()> dispatch_handler);

    /** Деструктор. Останавливает Tcl, рвёт подписки. */
    ~prompt_service();

    /**
     * Прерывает активный Tcl-скрипт сессии. Вызывается ssh_inetd на
     * завершении сервера, чтобы /while 1 {} не блокировал выход.
     */
    void interrupt() noexcept override;

    /**
     * \return MQTT-клиент сессии. Доступ для Tcl-команд через
     *         tcl->user_data() → prompt_service.
     */
    tim::mqtt_client &mqtt() noexcept;

    /**
     * \return Подключение к БД сессии. Доступ для Tcl-команд через
     *         tcl->user_data() → prompt_service.
     */
    tim::sqlite_db &db() noexcept;

    /**
     * \return UUID последнего сообщения, увиденного в этой сессии
     *         (от другого пользователя или из истории). Команда /react
     *         использует его как неявную цель.
     */
    const tim::uuid &last_seen_post() const noexcept;

private:

    /** PIMPL: терминал, Tcl, подписки, кэши. */
    std::unique_ptr<tim::p::prompt_service> _d;
};

}
