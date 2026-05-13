#include "tim_sqlite_query.h"

#include "tim_sqlite_query_p.h"

#include "tim_config.h"
#include "tim_sqlite_db.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include <cassert>
#include <thread>


// Открытые

/**
 * Конструирует запрос привязанный к БД, с заданным SQL.
 * prepare() пока не вызывается.
 *
 * \param db Указатель на открытую БД. Должна жить дольше запроса.
 * \param sql SQL-текст с '?' или ':name' плейсхолдерами.
 */
tim::sqlite_query::sqlite_query(const tim::sqlite_db *db, const std::string &sql)
    : _d(db)
{
    assert(!sql.empty());

    _d->_sql = sql;
}

/** Деструктор. Освобождает sqlite3_stmt. */
tim::sqlite_query::~sqlite_query()
{
    sqlite3_finalize(_d->_stmt);
}

/** \return Исходный SQL-текст. */
const std::string &tim::sqlite_query::sql() const
{
    return _d->_sql;
}

/** \return true, если sqlite3_stmt уже создан (prepare() был вызван). */
bool tim::sqlite_query::prepared() const
{
    return _d->_prepared;
}

/**
 * Готовит SQL к выполнению (sqlite3_prepare_v2). Идемпотентно:
 * повторный вызов на готовом запросе — no-op.
 *
 * \return true при успехе, false при ошибке (залогирована).
 */
bool tim::sqlite_query::prepare()
{
    assert(!_d->_stmt && "The query is prepared already.");

    int res = SQLITE_OK;
    const char *err_msg;
    for (unsigned count = 0; count < tim::DB_BUSY_TRIES; ++count)
        switch ((res = sqlite3_prepare_v2(_d->_db->sqlite(),
                                          _d->_sql.c_str(),
                                          (int)_d->_sql.size() + 1,
                                          &_d->_stmt,
                                          &err_msg)))
        {
            case SQLITE_OK:
                _d->_prepared = true;
                return true;

            case SQLITE_BUSY:
                TIM_TRACE(debug,
                         TIM_TR("Database '%s' is busy. Try #%u. Retrying in %ld microseconds."_en,
                               "База данных '%s' занята. Попытка №%u. Повторим попытку через %ld микросекунд."_ru),
                         _d->_db->path().string().c_str(),
                         count,
                         tim::DB_BUSY_TIMEOUT.count());
                std::this_thread::sleep_for(tim::DB_BUSY_TIMEOUT);
                break;

            default:
                goto failure;
        }

failure:

    return TIM_TRACE(error,
                    TIM_TR("Failed to prepare SQL query '%s' to database '%s': %s %s"_en,
                          "Ошибка при подготовке SQL-запроса '%s' к базе данных '%s': %s %s"_ru),
                    _d->_sql.c_str(),
                    _d->_db->path().string().c_str(),
                    sqlite3_errstr(res),
                    err_msg);
}

/**
 * Привязывает bool к ?-параметру по 1-based индексу.
 *
 * \param index 1-based индекс параметра.
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(int index, bool value)
{
    return bind(index, (int)value);
}

/**
 * Привязывает int к ?-параметру по 1-based индексу.
 *
 * \param index 1-based индекс параметра.
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(int index, int value)
{
    assert(_d->_stmt);

    const int res = sqlite3_bind_int(_d->_stmt, index, value);
    if (res != SQLITE_OK)
        return TIM_TRACE(error,
                        TIM_TR("Failed to bind an int value at index %d for SQL query '%s' to database '%s': %s"_en,
                              "Ошибка при привязке целочисленного значения к позиции %d для SQL-запроса '%s' к базе данных '%s': %s"_ru),
                        index,
                        _d->_sql.c_str(),
                        _d->_db->path().string().c_str(),
                        sqlite3_errstr(res));
    return true;
}

/**
 * Привязывает int64 к ?-параметру по 1-based индексу.
 *
 * \param index 1-based индекс параметра.
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(int index, std::int64_t value)
{
    assert(_d->_stmt);

    const int res = sqlite3_bind_int64(_d->_stmt, index, value);
    if (res != SQLITE_OK)
        return TIM_TRACE(error,
                        TIM_TR("Failed to bind a 64-bit integer value at index %d for SQL query '%s' to database '%s': %s"_en,
                              "Ошибка при привязке 64-битного целочисленного значения к позиции %d для SQL-запроса '%s' к базе данных '%s': %s"_ru),
                        index,
                        _d->_sql.c_str(),
                        _d->_db->path().string().c_str(),
                        sqlite3_errstr(res));

    return true;
}

/**
 * Привязывает double к ?-параметру по 1-based индексу.
 *
 * \param index 1-based индекс параметра.
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(int index, const double value)
{
    assert(_d->_stmt);

    const int res = sqlite3_bind_double(_d->_stmt, index, value);
    if (res != SQLITE_OK)
        return TIM_TRACE(error,
                        TIM_TR("Failed to bind a double value at index %d for SQL query '%s' to database '%s': %s"_en,
                              "Ошибка при привязке числового значения с плавающей запятой двойной точности к позиции %d для SQL-запроса '%s' к базе данных '%s': %s"_ru),
                        index,
                        _d->_sql.c_str(),
                        _d->_db->path().string().c_str(),
                        sqlite3_errstr(res));

    return true;
}

/**
 * Привязывает float к ?-параметру по 1-based индексу.
 *
 * \param index 1-based индекс параметра.
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(int index, float value)
{
    return bind(index, (double)value);
}

/**
 * Привязывает C-строку к ?-параметру по 1-based индексу.
 *
 * \param index 1-based индекс параметра.
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(int index, const char *value)
{
    assert(value);
    assert(_d->_stmt);

    const int res = sqlite3_bind_text(_d->_stmt, index, value, (int)std::strlen(value), SQLITE_TRANSIENT);
    if (res != SQLITE_OK)
        return TIM_TRACE(error,
                        TIM_TR("Failed to bind a string value at index %d for SQL query '%s' to database '%s': %s"_en,
                              "Ошибка при привязке строкового значения к позиции %d для SQL-запроса '%s' к базе данных '%s': %s"_ru),
                        index,
                        _d->_sql.c_str(),
                        _d->_db->path().string().c_str(),
                        sqlite3_errstr(res));

    return true;
}

/**
 * Привязывает std::string к ?-параметру по 1-based индексу.
 *
 * \param index 1-based индекс параметра.
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(int index, const std::string &value)
{
    assert(_d->_stmt);

    const int res = sqlite3_bind_text(_d->_stmt, index, value.c_str(), (int)value.size(), SQLITE_TRANSIENT);
    if (res != SQLITE_OK)
        return TIM_TRACE(error,
                        TIM_TR("Failed to bind a string value at index %d for SQL query '%s' to database '%s': %s"_en,
                              "Ошибка при привязке строкового значения к позиции %d для SQL-запроса '%s' к базе данных '%s': %s"_ru),
                        index,
                        _d->_sql.c_str(),
                        _d->_db->path().string().c_str(),
                        sqlite3_errstr(res));

    return true;
}

/**
 * Привязывает JSON-значение к ?-параметру по 1-based индексу.
 * Числовые и булевы значения сохраняются как соответствующие
 * примитивы; остальное — как сериализованная строка.
 *
 * \param index 1-based индекс параметра.
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(int index, const nlohmann::json &value)
{
    switch (value.type())
    {
        case nlohmann::detail::value_t::number_integer:
            return bind(index, value.get<std::int64_t>());

        case nlohmann::detail::value_t::number_unsigned:
            return bind(index, (std::int64_t)value.get<std::uint64_t>());

        case nlohmann::detail::value_t::number_float:
            return bind(index, value.get<float>());

        case nlohmann::detail::value_t::boolean:
            return bind(index, value.get<bool>()
                                    ? "true"
                                    : "false");

        default:
            return bind(index, value.dump());
    }

    return false;
}

/**
 * Привязывает bool к :name-параметру.
 *
 * \param key Имя параметра (без ':' префикса).
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(const std::string &key, bool value)
{
    assert(_d->_stmt);

    return bind(sqlite3_bind_parameter_index(_d->_stmt, key.c_str()), value);
}

/**
 * Привязывает int к :name-параметру.
 *
 * \param key Имя параметра (без ':' префикса).
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(const std::string &key, int value)
{
    assert(_d->_stmt);

    return bind(sqlite3_bind_parameter_index(_d->_stmt, key.c_str()), value);
}

/**
 * Привязывает int64 к :name-параметру.
 *
 * \param key Имя параметра (без ':' префикса).
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(const std::string &key, std::int64_t value)
{
    assert(_d->_stmt);

    return bind(sqlite3_bind_parameter_index(_d->_stmt, key.c_str()), value);
}

/**
 * Привязывает double к :name-параметру.
 *
 * \param key Имя параметра (без ':' префикса).
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(const std::string &key, double value)
{
    assert(_d->_stmt);

    return bind(sqlite3_bind_parameter_index(_d->_stmt, key.c_str()), value);
}

/**
 * Привязывает float к :name-параметру.
 *
 * \param key Имя параметра (без ':' префикса).
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(const std::string &key, float value)
{
    return bind(key, (double)value);
}

/**
 * Привязывает C-строку к :name-параметру.
 *
 * \param key Имя параметра (без ':' префикса).
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(const std::string &key, const char *value)
{
    assert(_d->_stmt);

    return bind(sqlite3_bind_parameter_index(_d->_stmt, key.c_str()), value);
}

/**
 * Привязывает std::string к :name-параметру.
 *
 * \param key Имя параметра (без ':' префикса).
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(const std::string &key, const std::string &value)
{
    assert(_d->_stmt);

    return bind(sqlite3_bind_parameter_index(_d->_stmt, key.c_str()), value);
}

/**
 * Привязывает JSON-значение к :name-параметру.
 *
 * \param key Имя параметра (без ':' префикса).
 * \param value Значение для привязки.
 * \return true при успехе.
 */
bool tim::sqlite_query::bind(const std::string &key, const nlohmann::json &value)
{
    assert(_d->_stmt);

    return bind(sqlite3_bind_parameter_index(_d->_stmt, key.c_str()), value);
}

/**
 * Сбрасывает все привязки. Полезно перед повторным exec()
 * с новыми параметрами.
 *
 * \return true при успехе.
 */
bool tim::sqlite_query::clear_bindings()
{
    assert(_d->_stmt);

    const int res = sqlite3_clear_bindings(_d->_stmt);
    if (res != SQLITE_OK)
        return TIM_TRACE(error,
                        TIM_TR("Failed to clear bindings for query '%s' to database '%s': %s"_en,
                              "Ошибка при очистке привязок значений к позициям в запросе '%s' к базе данных '%s': %s"_ru),
                        _d->_sql.c_str(),
                        _d->_db->path().string().c_str(),
                        sqlite3_errstr(res));

    return true;
}

/**
 * Выполняет non-SELECT (INSERT/UPDATE/DELETE/DDL). Внутренне
 * делает sqlite3_step и проверяет SQLITE_DONE.
 *
 * \return true при успехе.
 */
bool tim::sqlite_query::exec()
{
    return next();
}

/**
 * Шаг по результату SELECT.
 *
 * \param done Если не nullptr: выставляется true, когда строк
 *             больше нет.
 * \return true при успешном шаге.
 */
bool tim::sqlite_query::next(bool *done)
{
    assert(_d->_stmt);

    int res = SQLITE_OK;
    for (unsigned count = 0; count < tim::DB_BUSY_TRIES; ++count)
        switch ((res = sqlite3_step(_d->_stmt)))
        {
            case SQLITE_ROW:
                if (done)
                    *done = false;
                return true;

            case SQLITE_DONE:
                if (done)
                    *done = true;
                return true;

            case SQLITE_MISUSE:
                return TIM_TRACE(error,
                                TIM_TR("Misuse of SQL query '%s' to database '%s': %s"_en,
                                      "Неверное использование SQL-запроса '%s' к базе данных '%s': %s"_ru),
                                sqlite3_expanded_sql(_d->_stmt),
                                _d->_db->path().string().c_str(),
                                sqlite3_errstr(res));

            case SQLITE_BUSY:
                TIM_TRACE(debug,
                         TIM_TR("Database '%s' is busy. Try #%u. Retrying in %ld microseconds."_en,
                               "База данных '%s' занята. Попытка №%u. Повторяем попытку через %ld микросекунд."_ru),
                         _d->_db->path().string().c_str(),
                         count,
                         tim::DB_BUSY_TIMEOUT.count());
                std::this_thread::sleep_for(tim::DB_BUSY_TIMEOUT);
                break;

            default:
                goto failure;
        }

failure:

    return TIM_TRACE(error,
                    TIM_TR("Failed to perform SQL query '%s' to database '%s': %s"_en,
                          "Ошибка при выполнении SQL-запроса '%s' к базе данных '%s': %s"_ru),
                    sqlite3_expanded_sql(_d->_stmt),
                    _d->_db->path().string().c_str(),
                    sqlite3_errstr(res));
}

/** \return Общее число столбцов в SELECT. */
std::size_t tim::sqlite_query::column_count() const
{
    assert(_d->_stmt);

    return std::max(0, sqlite3_column_count(_d->_stmt));
}

/** \return Число столбцов, имеющих фактические данные (BLOB/TEXT). */
std::size_t tim::sqlite_query::data_column_count() const
{
    assert(_d->_stmt);

    return std::max(0, sqlite3_data_count(_d->_stmt));
}

/**
 * Тип значения в столбце (sqlite3_column_type).
 *
 * \param index 0-based индекс столбца.
 * \return SQLITE_INTEGER / SQLITE_FLOAT / SQLITE_TEXT / SQLITE_BLOB / SQLITE_NULL.
 */
int tim::sqlite_query::type(std::size_t index) const
{
    assert(_d->_stmt);

    return sqlite3_column_type(_d->_stmt, index);
}

/** \return Значение столбца как int. \param index 0-based индекс. */
int tim::sqlite_query::to_int(int index) const
{
    assert(_d->_stmt);

    return sqlite3_column_int(_d->_stmt, index);
}

/** \return Значение столбца как int64. \param index 0-based индекс. */
int64_t tim::sqlite_query::to_int64(int index) const
{
    assert(_d->_stmt);

    return sqlite3_column_int64(_d->_stmt, index);
}

/** \return Значение столбца как double. \param index 0-based индекс. */
double tim::sqlite_query::to_double(int index) const
{
    assert(_d->_stmt);

    return sqlite3_column_double(_d->_stmt, index);
}

/** \return Значение столбца как std::string. \param index 0-based индекс. */
std::string tim::sqlite_query::to_string(int index) const
{
    assert(_d->_stmt);

    const char *s = (const char *)sqlite3_column_text(_d->_stmt, index);
    return s
                ? s
                : std::string{};
}

/**
 * Возвращает sqlite3_stmt в начальное состояние (sqlite3_reset),
 * сохраняя привязки. Готов к повторному exec()/next().
 *
 * \return true при успехе.
 */
bool tim::sqlite_query::reset()
{
    assert(_d->_stmt);

    const int res = sqlite3_reset(_d->_stmt);
    if (res != SQLITE_OK)
        return TIM_TRACE(error,
                        TIM_TR("Failed to reset SQL query '%s' to database '%s': %s"_en,
                              "Ошибка при сбросе SQL-запроса '%s' к базе данных '%s': %s"_ru),
                        _d->_sql.c_str(),
                        _d->_db->path().string().c_str(),
                        sqlite3_errstr(res));
    return true;
}
