#pragma once

#include <cassert>
#include <string>


struct sqlite3_stmt;

namespace tim
{
    
class sqlite_db;

namespace p
{

struct sqlite_query
{
    explicit sqlite_query(const tim::sqlite_db *db)
        : _db(db)
    {
        assert(_db);
    }

    const tim::sqlite_db *const _db;
    sqlite3_stmt *_stmt = nullptr;
    std::string _sql;
    bool _prepared = false;
};

}

}
