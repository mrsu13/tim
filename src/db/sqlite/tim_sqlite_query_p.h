#pragma once

#include <cassert>
#include <string>


struct sqlite3_stmt;

namespace tim
{

class sqlite_db;

namespace p
{

/**
 * Внутреннее состояние tim::sqlite_query (PIMPL).
 */
struct sqlite_query
{
    /**
     * \param db Указатель на БД, к которой привязан запрос. Не должен
     *           быть nullptr.
     */
    explicit sqlite_query(const tim::sqlite_db *db)
        : _db(db)
    {
        assert(_db);
    }

    /** БД, к которой привязан запрос. */
    const tim::sqlite_db *const _db;
    /** Подготовленный sqlite3_stmt. */
    sqlite3_stmt *_stmt = nullptr;
    /** SQL-текст запроса. */
    std::string _sql;
    /** true, если prepare() прошёл успешно. */
    bool _prepared = false;
};

}

}
