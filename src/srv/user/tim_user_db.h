#pragma once

#include <string>


namespace tim
{

class sqlite_db;

/**
 * Гарантирует наличие строки в таблице user с заданным id
 * (INSERT OR IGNORE INTO user(id)).
 *
 * Используется перед операциями, у которых внешний ключ ссылается
 * на user(id): post.user_id, reaction.user_id, subscription.*_id.
 *
 * \param db Открытое подключение к БД.
 * \param user_id_canon Канонизированный UUID пользователя
 *                     (tim::uuid::to_string()). Перенормализация здесь
 *                     не делается — вызывающий код обычно работает
 *                     со строковым представлением.
 * \return true, если строка существует после вызова (вставлена сейчас
 *         или была раньше); false при ошибке БД (уже залогирована).
 */
bool ensure_user(tim::sqlite_db &db, const std::string &user_id_canon);

}
