#include "tim_profile_cache.h"

#include "tim_profile_cache_p.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
#include "tim_mqtt_topics.h"
#include "tim_sqlite_query.h"
#include "tim_trace.h"
#include "tim_translator.h"


// Открытые

/**
 * Загружает профиль своего пользователя из БД и сохраняет ссылки
 * на mqtt/db для последующих подписок и lookup-ов.
 *
 * \param mqtt MQTT-клиент для wildcard-подписок (см. subscribe()).
 * \param db Подключение к БД для load_user().
 * \param self_id UUID пользователя сессии.
 */
tim::profile_cache::profile_cache(tim::mqtt_client &mqtt,
                                  tim::sqlite_db &db,
                                  const tim::uuid &self_id)
    : _d(mqtt, db)
{
    _d->_self = _d->load_user(self_id);
}

/** Деструктор; RAII-объекты mqtt_subscription закрывают подписки автоматически. */
tim::profile_cache::~profile_cache() = default;

/**
 * \return Ссылка на профиль своего пользователя. Поля обновляются
 *         в реальном времени при приходе своих setnick/seticon/setmotto.
 */
const tim::user &tim::profile_cache::self() const noexcept
{
    return _d->_self;
}

/**
 * Возвращает профиль произвольного пользователя.
 *
 * \param id UUID пользователя. Для self_id возвращается self().
 * \return Кэшированная запись либо подгрузка из БД при первом
 *         обращении. Если строки в БД нет, возвращается user
 *         с пустыми ник/иконка/девиз и только заполненным id.
 */
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

/**
 * Сбрасывает кэш чужих профилей; self сохраняется.
 * Вызывать при восстановлении соединения, когда события профилей
 * могли быть пропущены.
 */
void tim::profile_cache::invalidate()
{
    _d->_known.clear();
}

/**
 * Оформляет подписки на wildcard-топики setnick/seticon/setmotto.
 * Вызывать при первом подключении и при каждом восстановлении
 * соединения. Прежние подписки (mqtt_subscription) сбрасываются
 * перед повторным оформлением.
 */
void tim::profile_cache::subscribe()
{
    // Сбрасываем прежние объекты перед повторным оформлением: при
    // восстановлении соединения mqtt_client уже утратил прежние подписки,
    // и хранить mqtt_subscription с устаревшими id незачем.
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
