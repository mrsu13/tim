#include "tim_user_service.h"

#include "tim_user_db.h"
#include "tim_user_service_p.h"
#include "tim_uuid.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topics.h"
#include "tim_mqtt_topic.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_sqlite_tx.h"
#include "tim_string_tools.h"
#include "tim_trace.h"
#include "tim_translator.h"


// Public

/**
 * Конструирует сервис и подписывается на MQTT.
 *
 * \param mqtt MQTT-клиент; должен жить дольше сервиса.
 * \param db Подключение к БД; должно жить дольше сервиса.
 */
tim::user_service::user_service(tim::mqtt_client &mqtt, tim::sqlite_db &db)
    : tim::service("user")
    , _d(new tim::p::user_service(mqtt, db))
{
    // Повторная выдача подписок на каждом (ре)подключении.
    _d->_on_connected = mqtt.connected.connect(
        [d = _d.get()]{ d->subscribe(); });

    if (mqtt.is_connected())
        _d->subscribe();
}

/** Деструктор. */
tim::user_service::~user_service() = default;


// Private

/**
 * Выдаёт семь MQTT-подписок (user/connect, setnick/+, seticon/+,
 * setmotto/+, setpubkey/+, subscribe/+, unsubscribe/+). Каждая лямбда —
 * тонкая прокладка к соответствующему обработчику p::user_service.
 */
void tim::p::user_service::subscribe()
{
    // user/connect — payload UUID; ensure_user.
    _sub_connect = _mqtt.subscribe(tim::topics::USER_CONNECT,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { connect(topic, data, size); });

    // user/setnick/<uuid> — payload nick; UPDATE user.nick + проверка коллизии.
    _sub_setnick = _mqtt.subscribe(tim::topics::USER_SETNICK_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { setnick(topic, data, size); });

    // user/seticon/<uuid> — payload icon; UPDATE user.icon.
    _sub_seticon = _mqtt.subscribe(tim::topics::USER_SETICON_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { seticon(topic, data, size); });

    // user/setmotto/<uuid> — payload motto; UPDATE user.motto.
    _sub_setmotto = _mqtt.subscribe(tim::topics::USER_SETMOTTO_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { setmotto(topic, data, size); });

    // user/setpubkey/<uuid> — payload OpenSSH-pub_key; UPDATE user.pub_key.
    _sub_setpubkey = _mqtt.subscribe(tim::topics::USER_SETPUBKEY_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { setpubkey(topic, data, size); });

    // user/subscribe/<subscriber> — payload publisher; INSERT subscription.
    _sub_subscribe = _mqtt.subscribe(tim::topics::USER_SUBSCRIBE_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { subscribe_to(topic, data, size); });

    // user/unsubscribe/<subscriber> — payload publisher; DELETE subscription.
    _sub_unsubscribe = _mqtt.subscribe(tim::topics::USER_UNSUBSCRIBE_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { unsubscribe_from(topic, data, size); });
}

/**
 * Обработчик user/connect: нормализует UUID к canonical и вызывает
 * ensure_user — гарантирует, что у пользователя есть строка в user.
 * \a topic не используется (фиксированный фильтр).
 */
void tim::p::user_service::connect(const tim::mqtt_topic &topic,
                                   const char *data, std::size_t size)
{
    (void) topic;

    // Нормализуем формат UUID — независимо от того, как пользователь
    // прислан (Canonical / NoBrackets), в БД храним только Canonical.
    const tim::uuid uid = std::string(data, size);
    if (!uid.valid())
    {
        TIM_TRACE(warning, "Ignoring user/connect with invalid UUID payload.");
        return;
    }
    const std::string uid_canon = uid.to_string();

    TIM_TRACE(debug, "User '%s' connected.", uid_canon.c_str());

    tim::ensure_user(_db, uid_canon);
}

/**
 * Обработчик user/setnick/<uuid>: проверяет уникальность ника и пишет
 * UPDATE user SET nick. При коллизии шлёт уведомление через
 * session/notice/<uuid> вместо тихого "не сработало".
 */
void tim::p::user_service::setnick(const tim::mqtt_topic &topic,
                                   const char *data, std::size_t size)
{
    const tim::uuid user_id = std::string(topic.last_level());
    const std::string user_id_canon = user_id.to_string();
    const std::string nick(data, size);

    TIM_TRACE(debug, "Setting user nick for '%s' ...",
              user_id_canon.c_str());

    // Предварительная проверка коллизии ника. Уникальность также
    // гарантируется UNIQUE(nick) в схеме; этот SELECT даёт более
    // понятную ошибку до того, как UPDATE упадёт по ограничению.
    {
        tim::sqlite_query q(&_db,
                            "SELECT id FROM user WHERE nick = ? AND id != ?");
        if (!q.prepare())
            return;
        q.bind(1, nick);
        q.bind(2, user_id_canon);

        bool done = false;
        if (q.next(&done) && !done)
        {
            TIM_TRACE(warning,
                      TIM_TR("Nick '%s' is already taken; skipping setnick for user '%s'."_en,
                             "Ник '%s' уже занят; пропускаем setnick для пользователя '%s'."_ru),
                      nick.c_str(), user_id_canon.c_str());
            // Сообщаем пользователю через шину уведомлений сессии — иначе
            // setnick молча "не сработает" и человек не поймёт, почему.
            const std::string notice = tim::sprintf(
                TIM_TR("Nick '%s' is already taken."_en,
                       "Ник '%s' уже занят."_ru),
                nick.c_str());
            _mqtt.publish(tim::topics::session_notice(user_id), notice);
            return;
        }
    }

    tim::sqlite_query q(&_db,
                        "UPDATE user SET nick = ? WHERE id = ?");
    if (!q.prepare())
            return;

    q.bind(1, nick);
    q.bind(2, user_id_canon);

    if (!q.exec())
        TIM_TRACE(error,
                  TIM_TR("Failed to update nick for user '%s'."_en,
                         "Ошибка при обновлении ника у пользователя '%s'."_ru),
                  user_id_canon.c_str());

}

/**
 * Обработчик user/seticon/<uuid>: пишет UPDATE user SET icon.
 */
void tim::p::user_service::seticon(const tim::mqtt_topic &topic,
                                   const char *data, std::size_t size)
{
    const tim::uuid user_id = std::string(topic.last_level());

    TIM_TRACE(debug, "Setting user icon for '%s' ...",
              user_id.to_string().c_str());

    tim::sqlite_query q(&_db,
                        "UPDATE user SET icon = ? WHERE id = ?");
    if (!q.prepare())
            return;

    const std::string icon(data, size);
    q.bind(1, icon);
    q.bind(2, user_id.to_string());

    if (!q.exec())
        TIM_TRACE(error,
                  TIM_TR("Failed to update icon for user '%s'."_en,
                         "Ошибка при обновлении иконки у пользователя '%s'."_ru),
                  user_id.to_string().c_str());

}

/**
 * Обработчик user/setmotto/<uuid>: пишет UPDATE user SET motto.
 */
void tim::p::user_service::setmotto(const tim::mqtt_topic &topic,
                                    const char *data, std::size_t size)
{
    const tim::uuid user_id = std::string(topic.last_level());

    TIM_TRACE(debug, "Setting motto for '%s' ...",
              user_id.to_string().c_str());

    tim::sqlite_query q(&_db,
                        "UPDATE user SET motto = ? WHERE id = ?");
    if (!q.prepare())
            return;

    const std::string motto(data, size);
    q.bind(1, motto);
    q.bind(2, user_id.to_string());

    if (!q.exec())
        TIM_TRACE(error,
                  TIM_TR("Failed to update motto for user '%s'."_en,
                         "Ошибка при обновлении девиза у пользователя '%s'."_ru),
                  user_id.to_string().c_str());
}

/**
 * Обработчик user/setpubkey/<uuid>: гарантирует существование пользователя
 * через ensure_user и пишет UPDATE user SET pub_key в единой транзакции.
 * UNIQUE(pub_key) защищает от привязки одного ключа к двум разным id.
 */
void tim::p::user_service::setpubkey(const tim::mqtt_topic &topic,
                                     const char *data, std::size_t size)
{
    const tim::uuid user_id = std::string(topic.last_level());
    const std::string user_id_canon = user_id.to_string();
    const std::string pub_key(data, size);

    TIM_TRACE(debug, "Setting pub_key for user '%s' ...",
              user_id_canon.c_str());

    // Гарантируем существование пользователя, затем пишем ключ. UNIQUE(pub_key)
    // обеспечивает, что один и тот же ключ не привязан к двум разным id;
    // совпадение id (один и тот же пользователь повторно подключился с тем же
    // ключом) штатно обрабатывается WHERE id = ?.
    tim::sqlite_tx tx(_db);
    if (!tx.active())
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to begin transaction for setpubkey."_en,
                         "Не удалось начать транзакцию для setpubkey."_ru));
        return;
    }

    if (!tim::ensure_user(_db, user_id_canon))
        return;

    {
        tim::sqlite_query q(&_db, "UPDATE user SET pub_key = ? WHERE id = ?");
        if (!q.prepare())
            return;
        q.bind(1, pub_key);
        q.bind(2, user_id_canon);
        if (!q.exec())
        {
            TIM_TRACE(error,
                      TIM_TR("Failed to set pub_key for user '%s'."_en,
                             "Не удалось установить pub_key пользователю '%s'."_ru),
                      user_id_canon.c_str());
            return;
        }
    }

    if (!tx.commit())
        TIM_TRACE(error,
                  TIM_TR("Failed to commit setpubkey for user '%s'."_en,
                         "Не удалось зафиксировать setpubkey для пользователя '%s'."_ru),
                  user_id_canon.c_str());
}

/**
 * Обработчик user/subscribe/<subscriber>: создаёт запись в subscription
 * с FK на обе стороны (ensure_user для обоих). Самоподписка отбрасывается.
 */
void tim::p::user_service::subscribe_to(const tim::mqtt_topic &topic,
                                        const char *data, std::size_t size)
{
    const tim::uuid subscriber = std::string(topic.last_level());
    const tim::uuid publisher = std::string(data, size);

    if (!subscriber.valid() || !publisher.valid() || subscriber == publisher)
    {
        TIM_TRACE(warning, "%s",
                  TIM_TR("Ignoring malformed user/subscribe event."_en,
                         "Игнорируем некорректное событие user/subscribe."_ru));
        return;
    }

    const std::string sub_canon = subscriber.to_string();
    const std::string pub_canon = publisher.to_string();

    TIM_TRACE(debug, "User '%s' subscribes to '%s'.",
              sub_canon.c_str(), pub_canon.c_str());

    // Гарантируем существование обеих сторон до INSERT-а в subscription
    // (внешние ключи требуют наличия и publisher_id, и subscriber_id).
    tim::sqlite_tx tx(_db);
    if (!tx.active())
    {
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to begin transaction for subscribe."_en,
                         "Не удалось начать транзакцию для subscribe."_ru));
        return;
    }

    if (!tim::ensure_user(_db, sub_canon) || !tim::ensure_user(_db, pub_canon))
        return;

    {
        tim::sqlite_query q(&_db,
                            "INSERT OR IGNORE INTO subscription (publisher_id, subscriber_id) VALUES (?, ?)");
        if (!q.prepare())
            return;
        q.bind(1, pub_canon);
        q.bind(2, sub_canon);
        if (!q.exec())
        {
            TIM_TRACE(error,
                      TIM_TR("Failed to record subscription '%s' -> '%s'."_en,
                             "Не удалось сохранить подписку '%s' -> '%s'."_ru),
                      sub_canon.c_str(), pub_canon.c_str());
            return;
        }
    }

    if (!tx.commit())
        TIM_TRACE(error, "%s",
                  TIM_TR("Failed to commit subscribe."_en,
                         "Не удалось зафиксировать subscribe."_ru));
}

/**
 * Обработчик user/unsubscribe/<subscriber>: DELETE из subscription
 * по паре (publisher_id, subscriber_id).
 */
void tim::p::user_service::unsubscribe_from(const tim::mqtt_topic &topic,
                                            const char *data, std::size_t size)
{
    const tim::uuid subscriber = std::string(topic.last_level());
    const tim::uuid publisher = std::string(data, size);

    if (!subscriber.valid() || !publisher.valid())
    {
        TIM_TRACE(warning, "%s",
                  TIM_TR("Ignoring malformed user/unsubscribe event."_en,
                         "Игнорируем некорректное событие user/unsubscribe."_ru));
        return;
    }

    const std::string sub_canon = subscriber.to_string();
    const std::string pub_canon = publisher.to_string();

    TIM_TRACE(debug, "User '%s' unsubscribes from '%s'.",
              sub_canon.c_str(), pub_canon.c_str());

    tim::sqlite_query q(&_db,
                        "DELETE FROM subscription WHERE publisher_id = ? AND subscriber_id = ?");
    if (!q.prepare())
        return;
    q.bind(1, pub_canon);
    q.bind(2, sub_canon);
    if (!q.exec())
        TIM_TRACE(error,
                  TIM_TR("Failed to remove subscription '%s' -> '%s'."_en,
                         "Не удалось удалить подписку '%s' -> '%s'."_ru),
                  sub_canon.c_str(), pub_canon.c_str());
}
