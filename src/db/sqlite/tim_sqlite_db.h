#pragma once

#include <filesystem>
#include <string>

#include "tim_pimpl.h"


struct sqlite3;

namespace tim
{

namespace p
{

struct sqlite_db;

}

/**
 * Тонкая обёртка вокруг sqlite3 для одного файла БД.
 *
 * Управляет жизненным циклом sqlite3*, поддерживает учитывающие счётчик ссылок
 * begin/commit (вложенные begin() инкрементируют счётчик, реальный
 * BEGIN/COMMIT выполняются только на внешнем уровне).
 */
class sqlite_db
{

public:

    sqlite_db();

    virtual ~sqlite_db();

    bool open(const std::filesystem::path &path);

    bool is_open() const;

    bool flush();

    void close();

    const std::filesystem::path &path() const;

#ifdef TIM_SQLITE_ENCRYPTION_ENABLED
    bool set_key(const std::string &key);

    bool rekey(const std::string &key);

    bool clear_key();
#endif

    static bool get_version(const std::filesystem::path &path, std::uint32_t &version);

    bool get_version(std::uint32_t &version) const;

    bool set_version(std::uint32_t version);

    virtual bool recreate();

    bool exec(const std::string &sql);

    bool exec_file(const std::filesystem::path &path);

    bool transaction(const std::string &sql);

    bool begin();

    bool begin(const std::string &save_point);

    bool is_transaction_active() const;

    bool commit();

    bool commit(const std::string &save_point);

    bool rollback();

    bool rollback(const std::string &save_point);

    bool vacuum();

    int change_count() const;

    sqlite3 *sqlite() const;

private:

    /** PIMPL: путь к файлу, sqlite3*, refcount транзакций. */
    tim::pimpl<tim::p::sqlite_db> _d;
};

}
