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

    /**
     * Конструирует объект и сразу открывает транзакцию в указанной БД.
     *
     * \param db Ссылка на открытое подключение к БД. Должна жить дольше,
     *           чем sqlite_tx.
     */
    explicit sqlite_tx(tim::sqlite_db &db);

    /**
     * Деструктор. Если begin() удался и commit() не вызывался —
     * автоматически делает rollback().
     */
    ~sqlite_tx();

    /** Копирование запрещено: транзакция привязана к конкретной БД. */
    sqlite_tx(const sqlite_tx &) = delete;
    /** Присваивание запрещено по той же причине. */
    sqlite_tx &operator=(const sqlite_tx &) = delete;

    /**
     * \return true, если begin() в конструкторе прошёл успешно
     *         и транзакция сейчас активна.
     */
    bool active() const;

    /**
     * Явно фиксирует транзакцию. Может быть вызван только один раз
     * на живом объекте.
     *
     * \return true при успехе, false если транзакция не активна или
     *         commit() уже вызывался; также false при ошибке db.commit().
     */
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
