#pragma once

#include "nlohmann/json.hpp"
#include "sqlite3.h"

#include <cstdint>
#include <string>
#include <memory>


namespace tim
{

class sqlite_db;

namespace p
{

struct sqlite_query;

}

/**
 * Обёртка над одним sqlite3_stmt: prepare → bind → exec/next → reset.
 *
 * Управляет жизненным циклом подготовленного запроса, предоставляет
 * перегруженные bind() для всех поддерживаемых типов параметров и
 * to_*() для извлечения данных из текущей строки результата.
 * Автоматически делает busy-retry при SQLITE_BUSY (см. DB_BUSY_TRIES /
 * DB_BUSY_TIMEOUT).
 */
class sqlite_query
{

public:

    /**
     * Конструирует запрос привязанный к БД, с заданным SQL.
     * prepare() пока не вызывается.
     *
     * \param db Указатель на открытую БД. Должна жить дольше запроса.
     * \param sql SQL-текст с '?' или ':name' плейсхолдерами.
     */
    sqlite_query(const tim::sqlite_db *db, const std::string &sql);

    /** Деструктор. Освобождает sqlite3_stmt. */
    ~sqlite_query();

    /** \return Исходный SQL-текст. */
    const std::string &sql() const;

    /** \return true, если sqlite3_stmt уже создан (prepare() был вызван). */
    bool prepared() const;

    /**
     * Готовит SQL к выполнению (sqlite3_prepare_v2). Идемпотентно:
     * повторный вызов на готовом запросе — no-op.
     *
     * \return true при успехе, false при ошибке (залогирована).
     */
    bool prepare();

    /** \name bind по индексу
     *  Привязывают значение к ?-параметру по 1-based индексу.
     *  \param index 1-based индекс параметра.
     *  \param value Значение для привязки.
     *  \return true при успехе.
     *  \{
     */
    bool bind(int index, bool value);
    bool bind(int index, int value);
    bool bind(int index, std::int64_t value);
    bool bind(int index, double value);
    bool bind(int index, float value);
    bool bind(int index, const char *value);
    bool bind(int index, const std::string &value);
    bool bind(int index, const nlohmann::json &value);
    /** \} */

    /** \name bind по имени
     *  Привязывают значение к :name-параметру.
     *  \param key Имя параметра (без ':' префикса).
     *  \param value Значение для привязки.
     *  \return true при успехе.
     *  \{
     */
    bool bind(const std::string &key, bool value);
    bool bind(const std::string &key, int value);
    bool bind(const std::string &key, std::int64_t value);
    bool bind(const std::string &key, double value);
    bool bind(const std::string &key, float value);
    bool bind(const std::string &key, const char *value);
    bool bind(const std::string &key, const std::string &value);
    bool bind(const std::string &key, const nlohmann::json &value);
    /** \} */

    /**
     * Сбрасывает все привязки. Полезно перед повторным exec()
     * с новыми параметрами.
     *
     * \return true при успехе.
     */
    bool clear_bindings();

    /**
     * Выполняет non-SELECT (INSERT/UPDATE/DELETE/DDL). Внутренне
     * делает sqlite3_step и проверяет SQLITE_DONE.
     *
     * \return true при успехе.
     */
    bool exec();

    /**
     * Шаг по результату SELECT.
     *
     * \param done Если не nullptr: выставляется true, когда строк
     *             больше нет.
     * \return true при успешном шаге.
     */
    bool next(bool *done = nullptr);

    /** \return Общее число столбцов в SELECT. */
    std::size_t column_count() const;

    /** \return Число столбцов, имеющих фактические данные (BLOB/TEXT). */
    std::size_t data_column_count() const;

    /**
     * Тип значения в столбце (sqlite3_column_type).
     *
     * \param index 0-based индекс столбца.
     * \return SQLITE_INTEGER / SQLITE_FLOAT / SQLITE_TEXT / SQLITE_BLOB / SQLITE_NULL.
     */
    int type(std::size_t index) const;

    /** \return Значение столбца как int. \param index 0-based индекс. */
    int to_int(int index) const;
    /** \return Значение столбца как int64. \param index 0-based индекс. */
    std::int64_t to_int64(int index) const;
    /** \return Значение столбца как float. \param index 0-based индекс. */
    float to_float(int index) const;
    /** \return Значение столбца как double. \param index 0-based индекс. */
    double to_double(int index) const;
    /** \return Значение столбца как std::string. \param index 0-based индекс. */
    std::string to_string(int index) const;
    /**
     * Парсит значение столбца как JSON.
     *
     * \param index 0-based индекс столбца.
     * \param ok Если не nullptr: true при успешном парсинге.
     * \return Распарсенный nlohmann::json; пустой при ошибке.
     */
    nlohmann::json to_json(int index, bool *ok = nullptr) const;

    /**
     * Возвращает sqlite3_stmt в начальное состояние (sqlite3_reset),
     * сохраняя привязки. Готов к повторному exec()/next().
     *
     * \return true при успехе.
     */
    bool reset();

private:

    /** PIMPL: sqlite3_stmt*, текст SQL, указатель на БД. */
    std::unique_ptr<tim::p::sqlite_query> _d;
};

}
