#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_user.h"

#include <unordered_map>


namespace tim
{

class mqtt_client;
class sqlite_db;

namespace p
{

/**
 * Внутреннее состояние tim::profile_cache (PIMPL).
 */
struct profile_cache
{
    /**
     * Запоминает ссылки на mqtt и db; сам кэш и self заполняет
     * внешний публичный конструктор.
     *
     * \param mqtt MQTT-клиент.
     * \param db Подключение к БД.
     */
    profile_cache(tim::mqtt_client &mqtt, tim::sqlite_db &db)
        : _mqtt(mqtt)
        , _db(db)
    {}

    /**
     * Загружает запись user(nick, icon, motto) из БД по id.
     *
     * \param id UUID искомого пользователя.
     * \return Заполненный tim::user; если строки в БД нет, возвращается
     *         объект с одним лишь id (остальные поля пусты).
     */
    tim::user load_user(const tim::uuid &id);

    /** MQTT-клиент сессии. */
    tim::mqtt_client &_mqtt;
    /** Подключение к БД. */
    tim::sqlite_db   &_db;

    /** Профиль своего пользователя сессии. */
    tim::user                                _self;
    /** Кэш профилей других пользователей (по UUID). */
    std::unordered_map<tim::uuid, tim::user> _known;

    // Подписки объявлены ПОСЛЕ полей-зависимостей (_mqtt, _db, _self,
    // _known) намеренно: при разрушении они уничтожаются ПЕРВЫМИ, и их
    // обработчики обращаются к этим полям. Перестановка проверяется
    // static_assert ниже.

    /** Подписка на user/setnick/+ — обновляет ники в кэше. */
    tim::mqtt_subscription _sub_setnick;
    /** Подписка на user/seticon/+ — обновляет иконки в кэше. */
    tim::mqtt_subscription _sub_seticon;
    /** Подписка на user/setmotto/+ — обновляет девизы в кэше. */
    tim::mqtt_subscription _sub_setmotto;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
static_assert(offsetof(profile_cache, _sub_setnick) > offsetof(profile_cache, _known),
              "MQTT-подписки должны быть объявлены ПОСЛЕ полей-зависимостей "
              "(_mqtt, _db, _self, _known) — подписки уничтожаются первыми, и "
              "их обработчики читают эти поля.");
#pragma GCC diagnostic pop

}

}
