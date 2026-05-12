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

    /**
     * Конструирует сервис; подписывается на MQTT при наличии соединения,
     * иначе ждёт первого сигнала connected.
     *
     * \param mqtt MQTT-клиент; должен жить дольше сервиса.
     * \param db Подключение к БД; должно жить дольше сервиса.
     */
    post_service(tim::mqtt_client &mqtt, tim::sqlite_db &db);

    /** Деструктор. Подписки и сигналы закрываются через RAII-токены. */
    ~post_service();

private:

    /** PIMPL: состояние подписок и обработчиков. */
    std::unique_ptr<tim::p::post_service> _d;
};

}
