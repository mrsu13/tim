#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_user.h"
#include "tim_uuid.h"

#include <memory>


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
 * профилей прилетают как MQTT-события и сразу обновляют кэш, без
 * перезапросов в БД.
 *
 * invalidate() сбрасывает кэш чужих профилей; вызывать после реконнекта
 * к брокеру, поскольку пропущенные за время отсутствия события setnick/
 * seticon/setmotto могли оставить устаревшие записи. self остаётся
 * валидным — мы и так знаем свой профиль.
 */
class profile_cache
{

public:

    /**
     * Загружает профиль своего пользователя из БД и сохраняет ссылки
     * на mqtt/db для последующих подписок и lookup-ов.
     *
     * \param mqtt MQTT-клиент для wildcard-подписок (см. subscribe()).
     * \param db Подключение к БД для load_user().
     * \param self_id UUID пользователя сессии.
     */
    profile_cache(tim::mqtt_client &mqtt,
                  tim::sqlite_db &db,
                  const tim::uuid &self_id);

    /** Деструктор; mqtt_subscription-токены закрывают подписки автоматически. */
    ~profile_cache();

    /**
     * \return Ссылка на профиль своего пользователя. Поля обновляются
     *         в реальном времени при приходе своих setnick/seticon/setmotto.
     */
    const tim::user &self() const noexcept;

    /**
     * Возвращает профиль произвольного пользователя.
     *
     * \param id UUID пользователя. Для self_id возвращается self().
     * \return Кэшированная запись либо подгрузка из БД при первом
     *         обращении. Если строки в БД нет, возвращается user
     *         с пустыми ник/иконка/девиз и только заполненным id.
     */
    tim::user user_for(const tim::uuid &id);

    /**
     * Сбросить кэш чужих профилей. self остаётся.
     * Вызывать на реконнекте, когда события профилей могли быть
     * пропущены.
     */
    void invalidate();

    /**
     * Выдаёт подписки на wildcard-топики setnick/seticon/setmotto.
     * Вызывать при первом подключении и на каждом реконнекте.
     * Прежние подписки (mqtt_subscription) сбрасываются перед
     * повторной выдачей.
     */
    void subscribe();

private:

    /** PIMPL: всё состояние (включая подписки и кэш-карту). */
    std::unique_ptr<tim::p::profile_cache> _d;
};

}
