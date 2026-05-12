#include "tim_user_db.h"

#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_trace.h"
#include "tim_translator.h"


/**
 * Выполняет INSERT OR IGNORE INTO user(id) VALUES (?). Любая ошибка
 * подготовки или выполнения логируется на уровне error.
 */
bool tim::ensure_user(tim::sqlite_db &db, const std::string &user_id_canon)
{
    tim::sqlite_query q(&db, "INSERT OR IGNORE INTO user (id) VALUES (?)");
    if (!q.prepare())
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to prepare query '%s'."_en,
                         "Не могу подготовить запрос '%s' к базе данных."_ru),
                  q.sql().c_str());
        return false;
    }
    q.bind(1, user_id_canon);
    if (!q.exec())
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to ensure user '%s' exists."_en,
                         "Не удалось создать запись пользователя '%s'."_ru),
                  user_id_canon.c_str());
        return false;
    }
    return true;
}
