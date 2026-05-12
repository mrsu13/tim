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

/**
 * Сервис, обрабатывающий реакции пользователей на сообщения.
 *
 * Слушает топик react/<post-uuid>/<user-uuid> с payload-ом — целочисленным
 * весом реакции. Значение weight = 0 удаляет реакцию пользователя
 * на сообщение; ненулевое значение создаёт или заменяет её.
 * После успешной записи в БД публикует react_event/<post-uuid>/<user-uuid>
 * для оповещения подписчиков.
 */
class reaction_service : public tim::service
{

public:

    /**
     * Конструирует сервис и подписывается на REACT_FILTER.
     *
     * \param mqtt MQTT-клиент; должен жить дольше сервиса.
     * \param db Подключение к БД; должно жить дольше сервиса.
     */
    reaction_service(tim::mqtt_client &mqtt, tim::sqlite_db &db);

    /** Деструктор. */
    ~reaction_service();

private:

    /** PIMPL: подписка на REACT_FILTER. */
    std::unique_ptr<tim::p::reaction_service> _d;
};

}
