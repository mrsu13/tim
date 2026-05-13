#pragma once

#include <string>


namespace tim
{

class sqlite_db;

bool ensure_user(tim::sqlite_db &db, const std::string &user_id_canon);

}
