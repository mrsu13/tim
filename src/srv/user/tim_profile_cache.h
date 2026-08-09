#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_user.h"
#include "tim_uuid.h"

#include "tim_pimpl.h"


namespace tim
{

class mqtt_client;
class sqlite_db;

namespace p
{

struct profile_cache;

}

/**
 * Кэш профилей пользователей: ник/иконка/девиз.
 *
 * Хранит запись для "своего" пользователя сессии (self) и ленивый кэш
 * для остальных. Подписан на wildcard-топики
 * user/setnick|seticon|setmotto/+ — изменения чужих и собственного
 * профилей поступают как MQTT-события и немедленно обновляют кэш, без
 * повторных запросов к БД.
 *
 * invalidate() сбрасывает кэш чужих профилей; вызывать после
 * восстановления соединения с брокером, поскольку пропущенные события setnick/
 * seticon/setmotto могли оставить устаревшие записи. self остаётся
 * валидным — мы и так знаем свой профиль.
 */
class profile_cache
{

public:

    profile_cache(tim::mqtt_client &mqtt,
                  tim::sqlite_db &db,
                  const tim::uuid &self_id);

    ~profile_cache();

    const tim::user &self() const noexcept;

    tim::user user_for(const tim::uuid &id);

    void invalidate();

    void subscribe();

private:

    /** PIMPL: всё состояние (включая подписки и кэш-карту). */
    tim::pimpl<tim::p::profile_cache> _d;
};

}
