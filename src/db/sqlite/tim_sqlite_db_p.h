#pragma once

#include <tim_sqlite_db.h>


namespace tim::p
{

struct sqlite_db
{
    using db_ptr = std::unique_ptr<sqlite3, std::function<void(sqlite3 *)>>;

    static db_ptr open_db(const std::filesystem::path &path);
    static bool close_db(sqlite3 *db);

    static bool replicate(sqlite3 *dst, sqlite3 *src,
                          const int pages_per_step = -1,
                          tim::sqlite_db::backup_progress_fn fn = nullptr);

    static int trace(unsigned event, void *self, void *p, void *x);
    static int progress(void *self);

    std::filesystem::path _path;

    db_ptr _db;

    int _transaction_count = 0;
};

}
