#include "tim_sqlite_tx.h"

#include "tim_sqlite_db.h"


tim::sqlite_tx::sqlite_tx(tim::sqlite_db &db)
    : _db(db)
{
    _active = _db.begin();
}

tim::sqlite_tx::~sqlite_tx()
{
    if (_active && !_done)
        _db.rollback();
}

bool tim::sqlite_tx::active() const
{
    return _active;
}

bool tim::sqlite_tx::commit()
{
    if (!_active || _done)
        return false;
    _done = true;
    return _db.commit();
}
