#include "tim_sqlite_tx.h"

#include "tim_sqlite_db.h"


/**
 * Открывает транзакцию в \a db. При неудаче active() возвращает false
 * и деструктор ничего делать не будет.
 */
tim::sqlite_tx::sqlite_tx(tim::sqlite_db &db)
    : _db(db)
{
    _active = _db.begin();
}

/**
 * Если транзакция активна и не была явно зафиксирована — откатывает её.
 */
tim::sqlite_tx::~sqlite_tx()
{
    if (_active && !_done)
        _db.rollback();
}

/**
 * \return Статус транзакции: true, если begin() прошёл и commit() ещё
 *         не вызывался.
 */
bool tim::sqlite_tx::active() const
{
    return _active;
}

/**
 * Фиксирует транзакцию через db.commit() и помечает её завершённой,
 * чтобы деструктор не делал rollback. Повторные вызовы возвращают false.
 *
 * \return Результат db.commit() при первом успешном вызове; false
 *         в остальных случаях.
 */
bool tim::sqlite_tx::commit()
{
    if (!_active || _done)
        return false;
    _done = true;
    return _db.commit();
}
