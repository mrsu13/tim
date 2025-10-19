#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>


struct sqlite3;

namespace tim
{

namespace p
{

struct sqlite_db;

}

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

    using backup_progress_fn = std::function<void(int unprocessed_page_count, int total_page_count)>;
    bool backup(const std::filesystem::path &path, backup_progress_fn fn = nullptr) const;

private:

    std::unique_ptr<tim::p::sqlite_db> _d;
};

}
