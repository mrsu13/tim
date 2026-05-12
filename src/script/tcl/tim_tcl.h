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

/**
 * Реализация a_script_engine поверх LIL (Little Interpreted Language).
 *
 * Держит lil_t-сессию, переводит её ошибки на язык TIM (через таблицу
 * правил) и пробрасывает дисперс-обработчик в LIL_CALLBACK_DISPATCH —
 * чтобы внешние event-loop-ы продолжали тикать во время скриптов.
 */
class tcl : public tim::a_script_engine
{

public:

    /**
     * Конструктор. Создаёт LIL-сессию и регистрирует базовые команды
     * (puts, palette256 и др.) через tcl_add_general / tcl_add_term.
     *
     * \param term Терминал, в который команды вроде /puts пишут.
     */
    explicit tcl(tim::a_terminal *term);

    /** Деструктор. lil_free освобождает интерпретатор. */
    virtual ~tcl();

    /**
     * \return Внутренняя LIL-сессия. Нужна тем, кто регистрирует свои
     *         команды снаружи (модуль владельца, например prompt).
     *         Никакой защиты — лезть в lil_* нужно осознанно.
     */
    lil_t lil() const;

    /**
     * Сохраняет произвольный непрозрачный указатель.
     *
     * Команды Tcl могут достать его через user_data() и привести
     * к нужному типу — это позволяет держать tim::tcl независимым
     * от конкретного контекста, в котором он используется (например,
     * prompt_service).
     *
     * \param data Указатель пользовательских данных.
     */
    void set_user_data(void *data);

    /** \return Ранее сохранённый указатель пользовательских данных. */
    void *user_data() const;

    /**
     * Регистрирует обработчик запроса /quit. Владелец (prompt_service)
     * вешает сюда действие, которое закрывает СВОЁ соединение, не трогая
     * остальной сервер.
     *
     * \param handler Колбэк выхода.
     */
    void set_quit_handler(std::function<void()> handler);

    /** Вызывает обработчик /quit, если он зарегистрирован. */
    void request_quit();

    /**
     * Регистрирует обработчик "тика" между Tcl-операторами: LIL вызывает
     * его через свой DISPATCH-обработчик, чтобы пока скрипт работает
     * не зависали внешние event-loop-ы.
     *
     * \param handler Колбэк-тик.
     */
    void set_dispatch_handler(std::function<void()> handler);

    /** \return true, если сейчас идёт вызов eval(). */
    bool evaluating() const override;

    /**
     * Выполняет переданный Tcl-скрипт.
     *
     * \param program Исходный текст программы.
     * \param res Если не nullptr: туда записывается значение последнего
     *           выражения (lil_to_string результата).
     * \return true, если LIL не выставил ошибку.
     */
    bool eval(const std::string &program, std::string *res = nullptr) override;

    /** Прерывает выполнение текущего eval (lil_break_run). */
    void break_eval() override;

    /** \return Префикс приглашения ("► " по умолчанию). */
    const std::string &prompt() const override;

    /** \return Текст последней ошибки на текущем языке. */
    const std::string &error_msg() const override;

    /** \return Позиция ошибки в исходнике (UTF8-aware). */
    std::size_t error_pos() const override;

    /**
     * \return Список имён LIL-функций, начинающихся с \a prefix.
     *
     * \param prefix Префикс для автодополнения.
     */
    std::vector<std::string> complete(const std::string &prefix) const override;

    /** \return Зарезервированные ключевые слова (для LIL пуст). */
    std::unordered_set<std::string> keywords() const override;

    /** \return Все зарегистрированные функции LIL-сессии. */
    std::unordered_set<std::string> functions() const override;

private:

    /**
     * p::tcl — внутренняя реализация tim::tcl, держит часть его
     * приватного состояния (в т.ч. зарегистрированные обработчики).
     */
    friend struct tim::p::tcl;

    /** PIMPL: lil_t, обработчики, текущее состояние ошибки. */
    std::unique_ptr<tim::p::tcl> _d;
};

}
