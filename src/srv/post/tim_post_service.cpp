#include "tim_post_service.h"

#include "tim_post_service_p.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_uuid.h"


// Public

tim::post_service::post_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
    : tim::service("post")
    , _d(new tim::p::post_service(mqtt, db))
{
    _d->_on_connected = mqtt.connected.connect(
        [d = _d.get()]{ d->subscribe(); });

    if (mqtt.is_connected())
        _d->subscribe();
}

tim::post_service::~post_service() = default;


// Private

void tim::p::post_service::subscribe()
{
    _sub_post = _mqtt.subscribe("post/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_post(topic, data, size); });
}

void tim::p::post_service::on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size)
{
    const std::string user_id_str(topic.last_level());
    const tim::uuid user_id = user_id_str;

    tim::sqlite_query q(&_db,
                        "INSERT OR REPLACE INTO post (id, user_id, text) VALUES (?, ?, ?)");
    if (!q.prepare())
        TIM_TRACE(Fatal,
                  TIM_TR("Failed to prepare database query '%s'."_en,
                         "Не могу подготовить запрос '%s' к базе данных."_ru),
                  q.sql().c_str());
    q.bind(1, tim::uuid::create().to_string());
    q.bind(2, user_id.to_string());
    q.bind(3, std::string(data, size));
    if (!q.exec())
        TIM_TRACE(Error,
                  TIM_TR("Failed to save post '%s' to the database."_en,
                         "Ошибка при сохранении поста '%s' в базе данных."_ru),
                  user_id_str.c_str());
}
