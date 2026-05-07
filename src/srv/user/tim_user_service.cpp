#include "tim_user_service.h"

#include "tim_user_service_p.h"
#include "tim_uuid.h"

#include "tim_mqtt_client.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_trace.h"
#include "tim_translator.h"


// Public

tim::user_service::user_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
    : tim::service("user")
    , _d(new tim::p::user_service(mqtt, db))
{
    _d->_on_connected = mqtt.connected.connect(
        [d = _d.get()]{ d->subscribe(); });

    if (mqtt.is_connected())
        _d->subscribe();
}

tim::user_service::~user_service() = default;


// Private

void tim::p::user_service::subscribe()
{
    _sub_connect = _mqtt.subscribe("user/connect",
        [this](const std::string &topic, const char *data, std::size_t size)
        { connect(topic, data, size); });

    _sub_setnick = _mqtt.subscribe("user/setnick/+",
        [this](const std::string &topic, const char *data, std::size_t size)
        { setnick(topic, data, size); });

    _sub_seticon = _mqtt.subscribe("user/seticon/+",
        [this](const std::string &topic, const char *data, std::size_t size)
        { seticon(topic, data, size); });
}

void tim::p::user_service::connect(const std::string &topic,
                                   const char *data, std::size_t size)
{
    (void) topic;

    TIM_TRACE(Debug, "User '%*s' connected.", (int)size, data);

    tim::sqlite_query q(&_db,
                        "INSERT OR IGNORE INTO user (id) VALUES (?)");
    if (!q.prepare())
        TIM_TRACE(Fatal,
                  TIM_TR("Failed to prepare query '%s'."_en,
                         "Не могу подготовить запрос '%s' к базе данных."_ru),
                  q.sql().c_str());

    const std::string user_id(data, size);
    q.bind(1, user_id);
    if (!q.exec())
        TIM_TRACE(Error,
                  TIM_TR("Failed to create user '%s'."_en,
                         "Ошибка при создании пользователя '%s'."_ru),
                  user_id.c_str());
}

void tim::p::user_service::setnick(const std::string &topic,
                                   const char *data, std::size_t size)
{
    const tim::uuid user_id = topic.substr(topic.rfind('/') + 1);

    TIM_TRACE(Debug, "Setting user nick for '%s' ...",
              user_id.to_string().c_str());

    tim::sqlite_query q(&_db,
                        "UPDATE user SET nick = ? WHERE id = ?");
    if (!q.prepare())
            TIM_TRACE(Fatal,
                TIM_TR("Failed to prepare query '%s'."_en,
                       "Не могу подготовить запрос '%s' к базе данных."_ru),
                q.sql().c_str());

    const std::string nick(data, size);
    q.bind(1, nick);
    q.bind(2, user_id.to_string());

    if (!q.exec())
        TIM_TRACE(Error,
                  TIM_TR("Failed to update nick for user '%s'."_en,
                         "Ошибка при обновлении ника у пользователя '%s'."_ru),
                  user_id.to_string().c_str());

}

void tim::p::user_service::seticon(const std::string &topic,
                                   const char *data, std::size_t size)
{
    const tim::uuid user_id = topic.substr(topic.rfind('/') + 1);

    TIM_TRACE(Debug, "Setting user icon for '%s' ...",
              user_id.to_string().c_str());

    tim::sqlite_query q(&_db,
                        "UPDATE user SET icon = ? WHERE id = ?");
    if (!q.prepare())
            TIM_TRACE(Fatal,
                TIM_TR("Failed to prepare query '%s'."_en,
                       "Не могу подготовить запрос '%s' к базе данных."_ru),
                q.sql().c_str());

    const std::string icon(data, size);
    q.bind(1, icon);
    q.bind(2, user_id.to_string());

    if (!q.exec())
        TIM_TRACE(Error,
                  TIM_TR("Failed to update icon for user '%s'."_en,
                         "Ошибка при обновлении иконки у пользователя '%s'."_ru),
                  user_id.to_string().c_str());

}
