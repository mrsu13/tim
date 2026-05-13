#include "tim_reaction_service.h"

#include "tim_reaction_service_p.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
#include "tim_mqtt_topics.h"
#include "tim_sqlite_query.h"
#include "tim_string_tools.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_uuid.h"


// Открытые

/**
 * Конструирует сервис и подписывается на REACT_FILTER.
 *
 * \param mqtt MQTT-клиент; должен жить дольше сервиса.
 * \param db Подключение к БД; должно жить дольше сервиса.
 */
tim::reaction_service::reaction_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
    : tim::service("reaction")
    , _d(new tim::p::reaction_service(mqtt, db))
{
    // Повторная выдача подписки на каждом (ре)подключении.
    _d->_on_connected = mqtt.connected.connect(
        [d = _d.get()]{ d->subscribe(); });

    if (mqtt.is_connected())
        _d->subscribe();
}

/** Деструктор. */
tim::reaction_service::~reaction_service() = default;


// Закрытые

/**
 * Подписывается на REACT_FILTER и делегирует каждое сообщение в on_react().
 */
void tim::p::reaction_service::subscribe()
{
    // Прокладка между подпиской и обработчиком on_react().
    _sub_react = _mqtt.subscribe(tim::topics::REACT_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_react(topic, data, size); });
}

/**
 * Парсит topic "react/<post-uuid>/<user-uuid>" и payload-вес. weight = 0
 * трактуется как снятие реакции (DELETE); иное значение — INSERT OR REPLACE
 * по UNIQUE(post_id, user_id). После успешной записи публикует
 * react_event/<post>/<user> с тем же payload-весом.
 */
void tim::p::reaction_service::on_react(const tim::mqtt_topic &topic,
                                        const char *data, std::size_t size)
{
    // topic = react/<post-uuid>/<user-uuid>
    const std::string user_id_str(topic.last_level());
    const std::string post_id_str(topic.parent().last_level());

    const tim::uuid post_id(post_id_str);
    const tim::uuid user_id(user_id_str);
    if (!post_id.valid() || !user_id.valid())
    {
        TIM_TRACE(warning, "%s",
                  TIM_TR("Ignoring reaction with invalid UUID(s)."_en,
                         "Игнорируем реакцию с некорректным UUID."_ru));
        return;
    }

    bool ok = false;
    const int weight = tim::to_int(std::string(data, size), &ok);
    if (!ok)
    {
        TIM_TRACE(warning, "%s",
                  TIM_TR("Ignoring reaction with non-integer weight."_en,
                         "Игнорируем реакцию с нечисловым весом."_ru));
        return;
    }

    const std::string post_id_canon = post_id.to_string();
    const std::string user_id_canon = user_id.to_string();

    // weight = 0 трактуется как снятие реакции.
    if (weight == 0)
    {
        tim::sqlite_query q(&_db,
                            "DELETE FROM reaction WHERE post_id = ? AND user_id = ?");
        if (!q.prepare())
            return;
        q.bind(1, post_id_canon);
        q.bind(2, user_id_canon);
        if (!q.exec())
        {
            TIM_TRACE(error,
                      TIM_TR("Failed to remove reaction by '%s' on post '%s'."_en,
                             "Не удалось удалить реакцию пользователя '%s' на сообщение '%s'."_ru),
                      user_id_canon.c_str(), post_id_canon.c_str());
            return;
        }

        // Уведомляем подписчиков: реакция снята (weight=0).
        _mqtt.publish(tim::topics::react_event(post_id, user_id), "0");
        return;
    }

    // INSERT OR REPLACE по UNIQUE(post_id, user_id): повторная реакция
    // того же пользователя на то же сообщение заменяет предыдущую.
    tim::sqlite_query q(&_db,
                        "INSERT OR REPLACE INTO reaction (id, post_id, user_id, weight) VALUES (?, ?, ?, ?)");
    if (!q.prepare())
        return;
    q.bind(1, tim::uuid::create().to_string());
    q.bind(2, post_id_canon);
    q.bind(3, user_id_canon);
    q.bind(4, weight);
    if (!q.exec())
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to record reaction by '%s' on post '%s' (weight=%d)."_en,
                         "Не удалось сохранить реакцию пользователя '%s' на сообщение '%s' (вес=%d)."_ru),
                  user_id_canon.c_str(), post_id_canon.c_str(), weight);
        return;
    }

    // Уведомление в шину: отдельный топик от исходного react/+/+, чтобы
    // клиенты видели только подтверждённые (успешно записанные) реакции.
    _mqtt.publish(tim::topics::react_event(post_id, user_id),
                  std::to_string(weight));
}
