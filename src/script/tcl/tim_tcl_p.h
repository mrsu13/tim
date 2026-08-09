#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <string>


typedef struct _lil_t *lil_t;

namespace tim
{

class tcl;

namespace p
{

/**
 * Внутреннее состояние tim::tcl (PIMPL).
 *
 * Держит lil_t, обработчики (quit, dispatch), флаг evaluating и поля
 * для отчёта об ошибке.
 */
struct tcl
{
    /**
     * \param q Указатель на внешний tim::tcl (для callback-ов LIL,
     *          которые получают lil_t и достают this через lil_get_data).
     */
    explicit tcl(tim::tcl *q)
        : _q(q)
    {
        assert(_q);
    }

    /**
     * LIL_CALLBACK_WRITE: пишет байты, выданные puts/etc, в терминал.
     *
     * \param lil LIL-сессия.
     * \param msg C-строка для вывода.
     */
    static void write(lil_t lil, const char *msg);

    /**
     * LIL_CALLBACK_DISPATCH: такт между Tcl-операторами. Делегирует
     * на зарегистрированный _dispatch_handler.
     *
     * \param lil LIL-сессия.
     */
    static void dispatch(lil_t lil);

    /** Внешний tim::tcl-указатель (для callback-ов). */
    tim::tcl *const _q;

    /** LIL-сессия (lil_t). */
    lil_t _lil = nullptr;
    /** Непрозрачные данные владельца (см. tcl::set_user_data). */
    void *_user_data = nullptr;
    /** Обработчик /quit. */
    std::function<void()> _quit_handler;
    /** Обработчик такта между операторами. */
    std::function<void()> _dispatch_handler;
    /** true, если сейчас выполняется eval(). */
    bool _evaluating = false;
    /** Префикс приглашения командного интерпретатора. */
    std::string _prompt = "► ";
    /** Текст последней ошибки на выбранном языке. */
    std::string _error_msg;
    /** Позиция ошибки в исходнике (UTF8-aware). */
    std::size_t _error_pos = 0;
};

}

}
