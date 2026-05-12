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

/**
 * Сервис, обслуживающий профили пользователей: ник, иконка, девиз,
 * открытый ключ и подписки.
 *
 * Подписан на топики user/connect, user/setnick/+, user/seticon/+,
 * user/setmotto/+, user/setpubkey/+, user/subscribe/+, user/unsubscribe/+.
 * Каждое событие обновляет соответствующую запись/связь в БД.
 */
class user_service : public tim::service
{

public:

    /**
     * Конструирует сервис и подписывается на MQTT.
     *
     * \param mqtt MQTT-клиент; должен жить дольше сервиса.
     * \param db Подключение к БД; должно жить дольше сервиса.
     */
    user_service(tim::mqtt_client &mqtt, tim::sqlite_db &db);

    /** Деструктор. */
    ~user_service();

private:

    /** PIMPL: подписки на все семь топиков. */
    std::unique_ptr<tim::p::user_service> _d;
};

}
