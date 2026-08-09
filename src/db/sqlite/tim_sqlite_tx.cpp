#include "tim_sqlite_tx.h"

#include "tim_sqlite_db.h"


/**
 * Конструирует объект и сразу открывает транзакцию в указанной БД.
 * При неудаче active() возвращает false и деструктор ничего делать
 * не будет.
 *
 * \param db Ссылка на открытое подключение к БД. Должна жить дольше,
 *           чем sqlite_tx.
 */
tim::sqlite_tx::sqlite_tx(tim::sqlite_db &db)
    : _db(db)
{
    _active = _db.begin();
}

/**
 * Деструктор. Если begin() удался и commit() не вызывался —
 * автоматически делает rollback().
 */
tim::sqlite_tx::~sqlite_tx()
{
    if (_active && !_done)
        _db.rollback();
}

/**
 * \return true, если begin() в конструкторе прошёл успешно
 *         и транзакция сейчас активна.
 */
bool tim::sqlite_tx::active() const
{
    return _active;
}

/**
 * Явно фиксирует транзакцию через db.commit() и помечает её
 * завершённой, чтобы деструктор не делал rollback. Может быть вызван
 * только один раз на действующем объекте; повторные вызовы возвращают false.
 *
 * \return true при успехе, false если транзакция не активна или
 *         commit() уже вызывался; также false при ошибке db.commit().
 */
bool tim::sqlite_tx::commit()
{
    if (!_active || _done)
        return false;
    _done = true;
    return _db.commit();
}
