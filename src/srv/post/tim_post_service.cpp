#include "tim_post_service.h"

#include "tim_post_service_p.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
#include "tim_mqtt_topics.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_sqlite_tx.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_user_db.h"
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
    _sub_post = _mqtt.subscribe(tim::topics::POST_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_post(topic, data, size); });
}

void tim::p::post_service::on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size)
{
    // topic = post/<publisher-uuid>/<post-uuid>
    const std::string post_id_str(topic.last_level());
    const std::string publisher_id_str(topic.parent().last_level());

    const tim::uuid post_id(post_id_str);
    const tim::uuid publisher_id(publisher_id_str);
    if (!post_id.valid() || !publisher_id.valid())
    {
        TIM_TRACE(Warning, "%s",
                  TIM_TR("Ignoring post with invalid UUID(s)."_en,
                         "Игнорируем сообщение с некорректным UUID."_ru));
        return;
    }
    const std::string publisher_id_canon = publisher_id.to_string();
    const std::string post_id_canon = post_id.to_string();

    // Гарантируем наличие автора и сохраняем сообщение единой транзакцией —
    // FK post.user_id → user(id) требует, чтобы автор существовал.
    tim::sqlite_tx tx(_db);
    if (!tx.active())
    {
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to begin transaction for storing post."_en,
                         "Не удалось начать транзакцию для сохранения сообщения."_ru));
        return;
    }

    if (!tim::ensure_user(_db, publisher_id_canon))
        return;

    {
        tim::sqlite_query q(&_db,
                            "INSERT OR REPLACE INTO post (id, user_id, text) VALUES (?, ?, ?)");
        if (!q.prepare())
            TIM_TRACE(Fatal,
                      TIM_TR("Failed to prepare database query '%s'."_en,
                             "Не могу подготовить запрос '%s' к базе данных."_ru),
                      q.sql().c_str());
        q.bind(1, post_id_canon);
        q.bind(2, publisher_id_canon);
        q.bind(3, std::string(data, size));
        if (!q.exec())
        {
            TIM_TRACE(Error,
                      TIM_TR("Failed to save post '%s' to the database."_en,
                             "Ошибка при сохранении поста '%s' в базе данных."_ru),
                      post_id_canon.c_str());
            return;
        }
    }

    if (!tx.commit())
        TIM_TRACE(Error,
                  TIM_TR("Failed to commit transaction for post '%s'."_en,
                         "Не удалось зафиксировать транзакцию для сообщения '%s'."_ru),
                  post_id_canon.c_str());
}
