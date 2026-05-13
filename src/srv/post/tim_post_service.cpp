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

/**
 * Конструирует сервис; подписывается на MQTT при наличии соединения,
 * иначе ждёт первого сигнала connected.
 *
 * \param mqtt MQTT-клиент; должен жить дольше сервиса.
 * \param db Подключение к БД; должно жить дольше сервиса.
 */
tim::post_service::post_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
    : tim::service("post")
    , _d(mqtt, db)
{
    // Повторно выдаём подписки на каждое (ре)подключение к брокеру.
    _d->_on_connected = mqtt.connected.connect(
        [d = _d.get()]{ d->subscribe(); });

    if (mqtt.is_connected())
        _d->subscribe();
}

/** Деструктор. Подписки и сигналы закрываются через RAII-токены. */
tim::post_service::~post_service() = default;


// Private

/**
 * Подписывается на post/+/+ и делегирует каждое сообщение в on_post().
 */
void tim::p::post_service::subscribe()
{
    // Передаём содержимое в on_post — единственная "точка истины" для
    // парсинга topic-а и записи в БД.
    _sub_post = _mqtt.subscribe(tim::topics::POST_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_post(topic, data, size); });
}

/**
 * Парсит topic "post/<publisher-uuid>/<post-uuid>", гарантирует
 * существование автора в user (ensure_user) и пишет запись в post
 * единой транзакцией. INSERT OR IGNORE — при коллизии UUID нового
 * сообщения логируется warning, прежнее не затирается.
 */
void tim::p::post_service::on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size)
{
    // topic = post/<publisher-uuid>/<post-uuid>
    const std::string post_id_str(topic.last_level());
    const std::string publisher_id_str(topic.parent().last_level());

    const tim::uuid post_id(post_id_str);
    const tim::uuid publisher_id(publisher_id_str);
    if (!post_id.valid() || !publisher_id.valid())
    {
        TIM_TRACE(warning, "%s",
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
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to begin transaction for storing post."_en,
                         "Не удалось начать транзакцию для сохранения сообщения."_ru));
        return;
    }

    if (!tim::ensure_user(_db, publisher_id_canon))
        return;

    {
        // INSERT OR IGNORE (а не REPLACE): post.id — UUIDv4, коллизия
        // астрономически маловероятна, но если она всё-таки случится,
        // безопаснее сохранить более ранее сообщение и явно сообщить
        // о потере, чем тихо затереть его новым.
        tim::sqlite_query q(&_db,
                            "INSERT OR IGNORE INTO post (id, user_id, text) VALUES (?, ?, ?)");
        if (!q.prepare())
            return;
        q.bind(1, post_id_canon);
        q.bind(2, publisher_id_canon);
        q.bind(3, std::string(data, size));
        if (!q.exec())
        {
            TIM_TRACE(error,
                      TIM_TR("Failed to save post '%s' to the database."_en,
                             "Ошибка при сохранении поста '%s' в базе данных."_ru),
                      post_id_canon.c_str());
            return;
        }
        if (_db.change_count() == 0)
            TIM_TRACE(warning,
                      TIM_TR("Post UUID collision: '%s' already exists, dropping new post by '%s'."_en,
                             "Коллизия UUID сообщения: '%s' уже существует, новое сообщение от '%s' отброшено."_ru),
                      post_id_canon.c_str(), publisher_id_canon.c_str());
    }

    if (!tx.commit())
        TIM_TRACE(error,
                  TIM_TR("Failed to commit transaction for post '%s'."_en,
                         "Не удалось зафиксировать транзакцию для сообщения '%s'."_ru),
                  post_id_canon.c_str());
}
