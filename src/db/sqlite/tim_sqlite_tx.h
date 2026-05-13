#pragma once


namespace tim
{

class sqlite_db;

/**
 * RAII-обёртка над BEGIN/COMMIT/ROLLBACK.
 *
 * В конструкторе вызывает begin(); если не удалось — active() возвращает
 * false и деструктор ничего не делает. Если транзакция активна и не была
 * явно зафиксирована вызовом commit(), деструктор делает rollback().
 * Удобно для функций с несколькими early-return: можно просто `return`,
 * не повторяя rollback() в каждой ветке.
 */
class sqlite_tx
{

public:

    explicit sqlite_tx(tim::sqlite_db &db);

    ~sqlite_tx();

    /** Копирование запрещено: транзакция привязана к конкретной БД. */
    sqlite_tx(const sqlite_tx &) = delete;
    /** Присваивание запрещено по той же причине. */
    sqlite_tx &operator=(const sqlite_tx &) = delete;

    bool active() const;

    bool commit();

private:

    /** Ссылка на БД, в которой открыта транзакция. */
    tim::sqlite_db &_db;
    /** true, если begin() в конструкторе прошёл; иначе деструктор no-op. */
    bool            _active = false;
    /** true, если commit() уже вызвался; деструктор тогда не делает rollback. */
    bool            _done = false;
};

}
