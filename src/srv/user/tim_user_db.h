#pragma once

#include <string>


namespace tim
{

class sqlite_db;

// Гарантирует наличие строки в таблице user с заданным id (INSERT OR IGNORE).
// Возвращает true, если строка существует после вызова (вставлена сейчас или
// существовала ранее). Используется перед операциями, у которых внешний ключ
// ссылается на user(id): post.user_id, reaction.user_id, subscription.*_id.
//
// Принимает уже канонизированный UUID (tim::uuid::to_string()); вызывающий
// код обычно работает со строковым представлением, поэтому отдельно
// перенормализовывать здесь незачем.
bool ensure_user(tim::sqlite_db &db, const std::string &user_id_canon);

}
