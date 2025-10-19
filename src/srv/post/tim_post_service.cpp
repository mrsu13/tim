#include "tim_post_service.h"

#include "tim_post_service_p.h"

#include "tim_application.h"
#include "tim_mqtt_client.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_uuid.h"


// Public

tim::post_service::post_service()
    : tim::service("post")
    , _d(new tim::p::post_service())
{
    tim::app()->mqtt()->connected.connect(std::bind(&tim::p::post_service::subscribe, _d.get()));

    if (tim::app()->mqtt()->is_connected())
        _d->subscribe();
}

tim::post_service::~post_service() = default;


// Private

void tim::p::post_service::subscribe()
{
    tim::app()->mqtt()->subscribe("post/+",
                                  std::bind(&tim::p::post_service::on_post, this,
                                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void tim::p::post_service::on_post(const std::filesystem::path &topic, const char *data, std::size_t size)
{
    const tim::uuid user_id = topic.filename().string();

    tim::sqlite_query q(tim::app()->db(),
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
                  topic.filename().string().c_str());
}
