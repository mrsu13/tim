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

class sqlite_query
{

public:

    sqlite_query(const tim::sqlite_db *db, const std::string &sql);
    ~sqlite_query();

    const std::string &sql() const;

    bool prepared() const;
    bool prepare();

    bool bind(int index, bool value);
    bool bind(int index, int value);
    bool bind(int index, std::int64_t value);
    bool bind(int index, double value);
    bool bind(int index, float value);
    bool bind(int index, const char *value);
    bool bind(int index, const std::string &value);
    bool bind(int index, const nlohmann::json &value);

    bool bind(const std::string &key, bool value);
    bool bind(const std::string &key, int value);
    bool bind(const std::string &key, std::int64_t value);
    bool bind(const std::string &key, double value);
    bool bind(const std::string &key, float value);
    bool bind(const std::string &key, const char *value);
    bool bind(const std::string &key, const std::string &value);
    bool bind(const std::string &key, const nlohmann::json &value);

    bool clear_bindings();

    bool exec();
    bool next(bool *done = nullptr);

    std::size_t column_count() const;
    std::size_t data_column_count() const;

    int type(std::size_t index) const;

    int to_int(int index) const;
    std::int64_t to_int64(int index) const;
    float to_float(int index) const;
    double to_double(int index) const;
    std::string to_string(int index) const;
    nlohmann::json to_json(int index, bool *ok = nullptr) const;

    bool reset();

private:

    std::unique_ptr<tim::p::sqlite_query> _d;
};

}
