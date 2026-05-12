#pragma once

#include "tim_a_script_engine.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>


typedef struct _lil_t *lil_t;

namespace tim
{

namespace p
{

struct tcl;

}

class a_terminal;

class tcl : public tim::a_script_engine
{

public:

    explicit tcl(tim::a_terminal *term);
    virtual ~tcl();

    // Внутренняя LIL-сессия. Нужно тем, кто регистрирует свои команды
    // снаружи (модуль владельца, например prompt). Никакой защиты —
    // лезть в lil_* нужно осознанно.
    lil_t lil() const;

    // Произвольный непрозрачный указатель, который владелец сохраняет
    // в момент регистрации. Команды Tcl могут достать его через
    // user_data() и привести к нужному типу — это позволяет держать
    // tim::tcl независимым от конкретного контекста, в котором он
    // используется (например, prompt_service).
    void set_user_data(void *data);
    void *user_data() const;

    // Обработчик запроса /quit. Владелец (prompt_service) регистрирует
    // здесь действие, которое закрывает СВОЁ соединение, не трогая
    // остальной сервер.
    void set_quit_handler(std::function<void()> handler);
    void request_quit();

    // Обработчик "тика" между Tcl-операторами: LIL вызывает его через
    // свой DISPATCH-callback, чтобы пока скрипт работает не зависали
    // внешние event-loop-ы.
    void set_dispatch_handler(std::function<void()> handler);

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

    // p::tcl — внутренняя реализация tim::tcl и держит часть его
    // приватного состояния (в т.ч. зарегистрированные обработчики).
    friend struct tim::p::tcl;

    std::unique_ptr<tim::p::tcl> _d;
};

}
