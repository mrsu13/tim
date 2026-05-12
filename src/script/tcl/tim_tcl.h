#pragma once

#include "tim_a_script_engine.h"
#include "tim_uuid.h"

#include <cstddef>
#include <memory>
#include <string>


namespace tim
{

namespace p
{

struct tcl;

}

class a_terminal;
class mqtt_client;
class sqlite_db;

class tcl : public tim::a_script_engine
{

public:

    tcl(tim::a_terminal *term, const tim::uuid &user_id,
        tim::mqtt_client &mqtt, tim::sqlite_db &db);
    virtual ~tcl();

    const tim::uuid &user_id() const;
    tim::mqtt_client &mqtt() const;
    tim::sqlite_db &db() const;

    // UUID последнего сообщения, увиденного в чате этой сессии. Команды
    // (например /react) используют его как неявный аргумент. Пустой uuid
    // означает, что ещё ни одно чужое сообщение не получено.
    void set_last_post_id(const tim::uuid &post_id);
    const tim::uuid &last_post_id() const;

    bool evaluating() const override;
    bool eval(const std::string &program, std::string *res = nullptr) override;
    void break_eval() override;

    const std::string &prompt() const override;
    const std::string &error_msg() const override;
    std::size_t error_pos() const override;

    std::vector<std::string> complete(const std::string &prefix) const override;

    std::unordered_set<std::string> keywords() const override;
    std::unordered_set<std::string> functions() const override;

private:

    std::unique_ptr<tim::p::tcl> _d;
};

}
