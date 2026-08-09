#pragma once

#include "tim_a_script_engine.h"

#include <cstddef>
#include <functional>
#include <string>

#include "tim_pimpl.h"


typedef struct _lil_t *lil_t;

namespace tim
{

namespace p
{

struct tcl;

}

class a_terminal;

/**
 * Реализация a_script_engine поверх LIL (Little Interpreted Language).
 *
 * Держит lil_t-сессию, переводит её ошибки на язык TIM (через таблицу
 * правил) и передаёт dispatch-обработчик в LIL_CALLBACK_DISPATCH —
 * чтобы внешние циклы событий продолжали работать во время скриптов.
 */
class tcl : public tim::a_script_engine
{

public:

    explicit tcl(tim::a_terminal *term);

    virtual ~tcl();

    lil_t lil() const;

    void set_user_data(void *data);

    void *user_data() const;

    void set_quit_handler(std::function<void()> handler);

    void request_quit();

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

    /**
     * p::tcl — внутренняя реализация tim::tcl, держит часть его
     * приватного состояния (в т.ч. зарегистрированные обработчики).
     */
    friend struct tim::p::tcl;

    /** PIMPL: lil_t, обработчики, текущее состояние ошибки. */
    tim::pimpl<tim::p::tcl> _d;
};

}
