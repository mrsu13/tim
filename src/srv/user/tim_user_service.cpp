#include "tim_user_service.h"

#include "tim_user_service_p.h"
#include "tim_uuid.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
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
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { connect(topic, data, size); });

    _sub_setnick = _mqtt.subscribe("user/setnick/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { setnick(topic, data, size); });

    _sub_seticon = _mqtt.subscribe("user/seticon/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { seticon(topic, data, size); });

    _sub_setpubkey = _mqtt.subscribe("user/setpubkey/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { setpubkey(topic, data, size); });

    _sub_subscribe = _mqtt.subscribe("user/subscribe/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { subscribe_to(topic, data, size); });

    _sub_unsubscribe = _mqtt.subscribe("user/unsubscribe/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { unsubscribe_from(topic, data, size); });
}

void tim::p::user_service::connect(const tim::mqtt_topic &topic,
                                   const char *data, std::size_t size)
{
    (void) topic;

    // Нормализуем формат UUID — независимо от того, как пользователь
    // прислан (Canonical / NoBrackets), в БД храним только Canonical.
    const tim::uuid uid = std::string(data, size);
    if (!uid.valid())
    {
        TIM_TRACE(Warning, "Ignoring user/connect with invalid UUID payload.");
        return;
    }
    const std::string uid_canon = uid.to_string();

    TIM_TRACE(Debug, "User '%s' connected.", uid_canon.c_str());

    tim::sqlite_query q(&_db,
                        "INSERT OR IGNORE INTO user (id) VALUES (?)");
    if (!q.prepare())
        TIM_TRACE(Fatal,
                  TIM_TR("Failed to prepare query '%s'."_en,
                         "Не могу подготовить запрос '%s' к базе данных."_ru),
                  q.sql().c_str());

    q.bind(1, uid_canon);
    if (!q.exec())
        TIM_TRACE(Error,
                  TIM_TR("Failed to create user '%s'."_en,
                         "Ошибка при создании пользователя '%s'."_ru),
                  uid_canon.c_str());
}

void tim::p::user_service::setnick(const tim::mqtt_topic &topic,
                                   const char *data, std::size_t size)
{
    const tim::uuid user_id = std::string(topic.last_level());
    const std::string user_id_canon = user_id.to_string();
    const std::string nick(data, size);

    TIM_TRACE(Debug, "Setting user nick for '%s' ...",
              user_id_canon.c_str());

    // Предварительная проверка коллизии ника. Уникальность также
    // гарантируется UNIQUE(nick) в схеме; этот SELECT даёт более
    // понятную ошибку до того, как UPDATE упадёт по ограничению.
    {
        tim::sqlite_query q(&_db,
                            "SELECT id FROM user WHERE nick = ? AND id != ?");
        if (!q.prepare())
            TIM_TRACE(Fatal,
                      TIM_TR("Failed to prepare query '%s'."_en,
                             "Не могу подготовить запрос '%s' к базе данных."_ru),
                      q.sql().c_str());
        q.bind(1, nick);
        q.bind(2, user_id_canon);

        bool done = false;
        if (q.next(&done) && !done)
        {
            TIM_TRACE(Warning,
                      TIM_TR("Nick '%s' is already taken; skipping setnick for user '%s'."_en,
                             "Ник '%s' уже занят; пропускаем setnick для пользователя '%s'."_ru),
                      nick.c_str(), user_id_canon.c_str());
            return;
        }
    }

    tim::sqlite_query q(&_db,
                        "UPDATE user SET nick = ? WHERE id = ?");
    if (!q.prepare())
            TIM_TRACE(Fatal,
                TIM_TR("Failed to prepare query '%s'."_en,
                       "Не могу подготовить запрос '%s' к базе данных."_ru),
                q.sql().c_str());

    q.bind(1, nick);
    q.bind(2, user_id_canon);

    if (!q.exec())
        TIM_TRACE(Error,
                  TIM_TR("Failed to update nick for user '%s'."_en,
                         "Ошибка при обновлении ника у пользователя '%s'."_ru),
                  user_id_canon.c_str());

}

void tim::p::user_service::seticon(const tim::mqtt_topic &topic,
                                   const char *data, std::size_t size)
{
    const tim::uuid user_id = std::string(topic.last_level());

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

void tim::p::user_service::setpubkey(const tim::mqtt_topic &topic,
                                     const char *data, std::size_t size)
{
    const tim::uuid user_id = std::string(topic.last_level());
    const std::string user_id_canon = user_id.to_string();
    const std::string pub_key(data, size);

    TIM_TRACE(Debug, "Setting pub_key for user '%s' ...",
              user_id_canon.c_str());

    // Гарантируем существование пользователя, затем пишем ключ. UNIQUE(pub_key)
    // обеспечивает, что один и тот же ключ не привязан к двум разным id;
    // совпадение id (один и тот же пользователь повторно подключился с тем же
    // ключом) штатно обрабатывается WHERE id = ?.
    if (!_db.begin())
    {
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to begin transaction for setpubkey."_en,
                         "Не удалось начать транзакцию для setpubkey."_ru));
        return;
    }

    {
        tim::sqlite_query q(&_db, "INSERT OR IGNORE INTO user (id) VALUES (?)");
        if (!q.prepare())
            TIM_TRACE(Fatal,
                      TIM_TR("Failed to prepare query '%s'."_en,
                             "Не могу подготовить запрос '%s' к базе данных."_ru),
                      q.sql().c_str());
        q.bind(1, user_id_canon);
        if (!q.exec())
        {
            TIM_TRACE(Error,
                      TIM_TR("Failed to ensure user '%s' exists for setpubkey."_en,
                             "Не удалось создать пользователя '%s' для setpubkey."_ru),
                      user_id_canon.c_str());
            _db.rollback();
            return;
        }
    }

    {
        tim::sqlite_query q(&_db, "UPDATE user SET pub_key = ? WHERE id = ?");
        if (!q.prepare())
            TIM_TRACE(Fatal,
                      TIM_TR("Failed to prepare query '%s'."_en,
                             "Не могу подготовить запрос '%s' к базе данных."_ru),
                      q.sql().c_str());
        q.bind(1, pub_key);
        q.bind(2, user_id_canon);
        if (!q.exec())
        {
            TIM_TRACE(Error,
                      TIM_TR("Failed to set pub_key for user '%s'."_en,
                             "Не удалось установить pub_key пользователю '%s'."_ru),
                      user_id_canon.c_str());
            _db.rollback();
            return;
        }
    }

    if (!_db.commit())
        TIM_TRACE(Error,
                  TIM_TR("Failed to commit setpubkey for user '%s'."_en,
                         "Не удалось зафиксировать setpubkey для пользователя '%s'."_ru),
                  user_id_canon.c_str());
}

void tim::p::user_service::subscribe_to(const tim::mqtt_topic &topic,
                                        const char *data, std::size_t size)
{
    const tim::uuid subscriber = std::string(topic.last_level());
    const tim::uuid publisher = std::string(data, size);

    if (!subscriber.valid() || !publisher.valid() || subscriber == publisher)
    {
        TIM_TRACE(Warning, "%s",
                  TIM_TR("Ignoring malformed user/subscribe event."_en,
                         "Игнорируем некорректное событие user/subscribe."_ru));
        return;
    }

    const std::string sub_canon = subscriber.to_string();
    const std::string pub_canon = publisher.to_string();

    TIM_TRACE(Debug, "User '%s' subscribes to '%s'.",
              sub_canon.c_str(), pub_canon.c_str());

    // Гарантируем существование обеих сторон до INSERT-а в subscription
    // (внешние ключи требуют наличия и publisher_id, и subscriber_id).
    if (!_db.begin())
    {
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to begin transaction for subscribe."_en,
                         "Не удалось начать транзакцию для subscribe."_ru));
        return;
    }

    auto ensure_user = [this](const std::string &id) -> bool
    {
        tim::sqlite_query q(&_db, "INSERT OR IGNORE INTO user (id) VALUES (?)");
        if (!q.prepare())
            return false;
        q.bind(1, id);
        return q.exec();
    };

    if (!ensure_user(sub_canon) || !ensure_user(pub_canon))
    {
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to ensure users exist for subscribe."_en,
                         "Не удалось создать пользователей для subscribe."_ru));
        _db.rollback();
        return;
    }

    {
        tim::sqlite_query q(&_db,
                            "INSERT OR IGNORE INTO subscription (publisher_id, subscriber_id) VALUES (?, ?)");
        if (!q.prepare())
            TIM_TRACE(Fatal,
                      TIM_TR("Failed to prepare query '%s'."_en,
                             "Не могу подготовить запрос '%s' к базе данных."_ru),
                      q.sql().c_str());
        q.bind(1, pub_canon);
        q.bind(2, sub_canon);
        if (!q.exec())
        {
            TIM_TRACE(Error,
                      TIM_TR("Failed to record subscription '%s' -> '%s'."_en,
                             "Не удалось сохранить подписку '%s' -> '%s'."_ru),
                      sub_canon.c_str(), pub_canon.c_str());
            _db.rollback();
            return;
        }
    }

    if (!_db.commit())
        TIM_TRACE(Error, "%s",
                  TIM_TR("Failed to commit subscribe."_en,
                         "Не удалось зафиксировать subscribe."_ru));
}

void tim::p::user_service::unsubscribe_from(const tim::mqtt_topic &topic,
                                            const char *data, std::size_t size)
{
    const tim::uuid subscriber = std::string(topic.last_level());
    const tim::uuid publisher = std::string(data, size);

    if (!subscriber.valid() || !publisher.valid())
    {
        TIM_TRACE(Warning, "%s",
                  TIM_TR("Ignoring malformed user/unsubscribe event."_en,
                         "Игнорируем некорректное событие user/unsubscribe."_ru));
        return;
    }

    const std::string sub_canon = subscriber.to_string();
    const std::string pub_canon = publisher.to_string();

    TIM_TRACE(Debug, "User '%s' unsubscribes from '%s'.",
              sub_canon.c_str(), pub_canon.c_str());

    tim::sqlite_query q(&_db,
                        "DELETE FROM subscription WHERE publisher_id = ? AND subscriber_id = ?");
    if (!q.prepare())
        TIM_TRACE(Fatal,
                  TIM_TR("Failed to prepare query '%s'."_en,
                         "Не могу подготовить запрос '%s' к базе данных."_ru),
                  q.sql().c_str());
    q.bind(1, pub_canon);
    q.bind(2, sub_canon);
    if (!q.exec())
        TIM_TRACE(Error,
                  TIM_TR("Failed to remove subscription '%s' -> '%s'."_en,
                         "Не удалось удалить подписку '%s' -> '%s'."_ru),
                  sub_canon.c_str(), pub_canon.c_str());
}
