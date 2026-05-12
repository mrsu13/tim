// Тесты RAII-обёртки tim::sqlite_tx над begin/commit/rollback.
//
// Используем настоящий tim::sqlite_db с временным файлом в /tmp/ —
// сценарии короткие, реальный sqlite даёт честную проверку семантики,
// без подделок поверх интерфейса.

#include "tim_test.h"

#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_sqlite_tx.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <unistd.h>


namespace
{

// Уникальный путь к временной БД на время одного тест-кейса.
struct tmp_db_path
{
    tmp_db_path()
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf),
                      "/tmp/tim_test_sqlite_tx_%ld_%d.db",
                      (long)getpid(),
                      ++counter());
        _p = buf;
    }
    ~tmp_db_path()
    {
        std::error_code ec;
        std::filesystem::remove(_p, ec);
        std::filesystem::remove(_p.string() + "-shm", ec);
        std::filesystem::remove(_p.string() + "-wal", ec);
    }

    const std::filesystem::path &path() const { return _p; }

    static int &counter()
    {
        static int n = 0;
        return n;
    }

    std::filesystem::path _p;
};

// Создаёт пустую таблицу t(v INTEGER) в открытой БД.
void create_table(tim::sqlite_db &db)
{
    db.exec("CREATE TABLE t (v INTEGER)");
}

// Сколько строк в t.
int row_count(tim::sqlite_db &db)
{
    tim::sqlite_query q(&db, "SELECT COUNT(*) FROM t");
    if (!q.prepare())
        return -1;
    bool done = false;
    if (!q.next(&done) || done)
        return -1;
    return q.to_int(0);
}

// Вставляет одну строку со значением v в t.
bool insert_row(tim::sqlite_db &db, int v)
{
    tim::sqlite_query q(&db, "INSERT INTO t (v) VALUES (?)");
    if (!q.prepare())
        return false;
    q.bind(1, v);
    return q.exec();
}

}


TIM_TEST_CASE(sqlite_tx_active_after_begin)
{
    tmp_db_path tmp;
    tim::sqlite_db db;
    TIM_CHECK(db.open(tmp.path()));

    tim::sqlite_tx tx(db);
    TIM_CHECK(tx.active());
    TIM_CHECK(db.is_transaction_active());
}

TIM_TEST_CASE(sqlite_tx_commit_persists_changes)
{
    tmp_db_path tmp;
    tim::sqlite_db db;
    TIM_CHECK(db.open(tmp.path()));
    create_table(db);

    {
        tim::sqlite_tx tx(db);
        TIM_CHECK(tx.active());
        TIM_CHECK(insert_row(db, 42));
        TIM_CHECK(tx.commit());
    }

    TIM_CHECK(row_count(db) == 1);
}

TIM_TEST_CASE(sqlite_tx_rollback_when_not_committed)
{
    tmp_db_path tmp;
    tim::sqlite_db db;
    TIM_CHECK(db.open(tmp.path()));
    create_table(db);

    // Транзакция уходит из скоупа без commit() — деструктор откатывает.
    {
        tim::sqlite_tx tx(db);
        TIM_CHECK(tx.active());
        TIM_CHECK(insert_row(db, 7));
    }

    TIM_CHECK(row_count(db) == 0);
    TIM_CHECK(!db.is_transaction_active());
}

TIM_TEST_CASE(sqlite_tx_commit_idempotent_second_call_returns_false)
{
    tmp_db_path tmp;
    tim::sqlite_db db;
    TIM_CHECK(db.open(tmp.path()));
    create_table(db);

    tim::sqlite_tx tx(db);
    TIM_CHECK(insert_row(db, 1));
    TIM_CHECK(tx.commit());
    // Повторный commit — false, без побочных эффектов.
    TIM_CHECK(!tx.commit());
    TIM_CHECK(row_count(db) == 1);
}

TIM_TEST_CASE(sqlite_tx_rollback_isolates_changes_from_separate_query)
{
    tmp_db_path tmp;
    tim::sqlite_db db;
    TIM_CHECK(db.open(tmp.path()));
    create_table(db);

    // Транзакция вставляет 3 строки, но не коммитится — после её выхода
    // из скоупа все три отката́ны одним rollback().
    {
        tim::sqlite_tx tx(db);
        TIM_CHECK(insert_row(db, 1));
        TIM_CHECK(insert_row(db, 2));
        TIM_CHECK(insert_row(db, 3));
        // Без commit().
    }

    TIM_CHECK(row_count(db) == 0);
}
