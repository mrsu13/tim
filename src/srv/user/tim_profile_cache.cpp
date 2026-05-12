#include "tim_profile_cache.h"

#include "tim_profile_cache_p.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
#include "tim_mqtt_topics.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_trace.h"
#include "tim_translator.h"


// Открытые

tim::profile_cache::profile_cache(tim::mqtt_client &mqtt,
                                  tim::sqlite_db &db,
                                  const tim::uuid &self_id)
    : _d(new tim::p::profile_cache(mqtt, db))
{
    _d->_self = _d->load_user(self_id);
}

tim::profile_cache::~profile_cache() = default;

const tim::user &tim::profile_cache::self() const noexcept
{
    return _d->_self;
}

tim::user tim::profile_cache::user_for(const tim::uuid &id)
{
    if (id == _d->_self.id)
        return _d->_self;

    const std::unordered_map<tim::uuid, tim::user>::iterator it = _d->_known.find(id);
    if (it != _d->_known.end())
        return it->second;

    tim::user u = _d->load_user(id);
    _d->_known.emplace(id, u);
    return u;
}

void tim::profile_cache::invalidate()
{
    _d->_known.clear();
}

void tim::profile_cache::subscribe()
{
    // Сбрасываем старые токены перед повторной выдачей: при реконнекте
    // mqtt_client уже потерял прежние подписки, и держать тут "висящие"
    // mqtt_subscription с устаревшими id незачем.
    _d->_sub_setnick = {};
    _d->_sub_seticon = {};
    _d->_sub_setmotto = {};

    _d->_sub_setnick = _d->_mqtt.subscribe(tim::topics::USER_SETNICK_FILTER,
        [d = _d.get()](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        {
            const tim::uuid uid = std::string(topic.last_level());
            if (!uid.valid())
                return;
            const std::string nick(data, size);
            if (uid == d->_self.id)
                d->_self.nick = nick;
            else
            {
                tim::user &u = d->_known[uid];
                u.id = uid;
                u.nick = nick;
            }
        });

    _d->_sub_seticon = _d->_mqtt.subscribe(tim::topics::USER_SETICON_FILTER,
        [d = _d.get()](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        {
            const tim::uuid uid = std::string(topic.last_level());
            if (!uid.valid())
                return;
            const std::string icon(data, size);
            if (uid == d->_self.id)
                d->_self.icon = icon;
            else
            {
                tim::user &u = d->_known[uid];
                u.id = uid;
                u.icon = icon;
            }
        });

    _d->_sub_setmotto = _d->_mqtt.subscribe(tim::topics::USER_SETMOTTO_FILTER,
        [d = _d.get()](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        {
            const tim::uuid uid = std::string(topic.last_level());
            if (!uid.valid())
                return;
            const std::string motto(data, size);
            if (uid == d->_self.id)
                d->_self.motto = motto;
            else
            {
                tim::user &u = d->_known[uid];
                u.id = uid;
                u.motto = motto;
            }
        });
}


// Закрытые

tim::user tim::p::profile_cache::load_user(const tim::uuid &id)
{
    tim::user u;
    u.id = id;

    tim::sqlite_query q(&_db, "SELECT nick, icon, motto FROM user WHERE id = ?");
    if (!q.prepare())
    {
        TIM_TRACE(warning,
                  TIM_TR("Failed to prepare query for loading user '%s'."_en,
                         "Не удалось подготовить запрос на загрузку пользователя '%s'."_ru),
                  id.to_string().c_str());
        return u;
    }
    q.bind(1, id.to_string());

    bool done = false;
    if (!q.next(&done) || done)
        return u;

    u.nick = q.to_string(0);
    u.icon = q.to_string(1);
    u.motto = q.to_string(2);
    return u;
}
