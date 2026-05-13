#pragma once

#include "tim_service.h"

#include "tim_pimpl.h"


namespace tim
{

class mqtt_client;
class sqlite_db;

namespace p
{

struct post_service;

}

/**
 * Сервис, принимающий и сохраняющий сообщения пользователей в БД.
 *
 * Подписан на фильтр POST_FILTER ("post/+/+"); при поступлении нового
 * сообщения гарантирует наличие автора в таблице user (ensure_user)
 * и пишет запись в post единой транзакцией.
 */
class post_service : public tim::service
{

public:

    post_service(tim::mqtt_client &mqtt, tim::sqlite_db &db);

    ~post_service();

private:

    /** PIMPL: состояние подписок и обработчиков. */
    tim::pimpl<tim::p::post_service> _d;
};

}
