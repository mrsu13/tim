#include "tim_tcl.h"

#include "tim_tcl_p.h"

#include "tim_a_protocol.h"
#include "tim_a_terminal.h"
#include "tim_string_tools.h"
#include "tim_translator.h"

// Commands (универсальные — относящиеся к самому скрипт-движку или
// терминалу). Команды, специфичные для чата, регистрирует владелец
// (например, prompt_service) после конструирования tim::tcl.
#include "tim_tcl_cmd_general.h"
#include "tim_tcl_cmd_term.h"

#include "lil.hpp"
#include "utf8/utf8.h"

#include <cassert>


// Public

/**
 * Конструктор. Создаёт LIL-интерпретатор, регистрирует callbacks
 * WRITE и DISPATCH, и добавляет базовые команды (puts, palette256 и т.п.).
 */
tim::tcl::tcl(tim::a_terminal *term)
    : tim::a_script_engine("Tcl", term)
    , _d(new tim::p::tcl(this))
{
    _d->_lil = lil_new();

    lil_callback(_d->_lil, LIL_CALLBACK_WRITE, (lil_callback_proc_t)tim::p::tcl::write);
    lil_callback(_d->_lil, LIL_CALLBACK_DISPATCH, (lil_callback_proc_t)tim::p::tcl::dispatch);

    tim::tcl_add_general(_d->_lil);
    tim::tcl_add_term(_d->_lil);
}

/** Деструктор. Освобождает LIL-интерпретатор. */
tim::tcl::~tcl()
{
    lil_free(_d->_lil);
}

/** \return Внутренний lil_t для регистрации команд снаружи. */
lil_t tim::tcl::lil() const
{
    return _d->_lil;
}

/** Сохраняет произвольный непрозрачный указатель. \param data Указатель. */
void tim::tcl::set_user_data(void *data)
{
    _d->_user_data = data;
}

/** \return Ранее сохранённый user_data. */
void *tim::tcl::user_data() const
{
    return _d->_user_data;
}

/** Запоминает обработчик /quit. \param handler Колбэк выхода. */
void tim::tcl::set_quit_handler(std::function<void()> handler)
{
    _d->_quit_handler = std::move(handler);
}

/** Вызывает зарегистрированный обработчик /quit, если он есть. */
void tim::tcl::request_quit()
{
    if (_d->_quit_handler)
        _d->_quit_handler();
}

/** Запоминает обработчик "тика" между Tcl-операторами. \param handler Колбэк. */
void tim::tcl::set_dispatch_handler(std::function<void()> handler)
{
    _d->_dispatch_handler = std::move(handler);
}

/** \return true, если сейчас идёт вызов eval(). */
bool tim::tcl::evaluating() const
{
    return _d->_evaluating;
}

/**
 * Выполняет переданный Tcl-скрипт через LIL. После lil_parse читает
 * сырое сообщение об ошибке, применяет таблицу RULES для подстановки
 * локализованных вариантов известных подстрок LIL, и добавляет
 * завершающую точку, если её ещё нет.
 */
bool tim::tcl::eval(const std::string &program, std::string *res)
{
    assert(!_d->_evaluating && "Recursive evaluating is not allowed.");

    TIM_TRACE(debug, "Evaluating '%s'", program.c_str());

    if (program.empty())
    {
        if (res)
            res->clear();
        return true;
    }

    _d->_evaluating = true;

    void *old_data = lil_get_data(_d->_lil);
    lil_set_data(_d->_lil, this);

    _d->_error_msg.clear();
    _d->_error_pos = 0;

    lil_value_t rv = lil_parse(_d->_lil, program.c_str(), program.size(), 1);
    const char *err_msg = nullptr;
    const bool ok = !lil_error(_d->_lil, &err_msg, &_d->_error_pos);
    _d->_error_pos = utf8nlen((const utf8_int8_t *)program.c_str(), _d->_error_pos);

    if (err_msg
            && *err_msg)
    {
        _d->_error_msg = err_msg;

        // Известные подстроки LIL → перевод. Порядок важен: "catcher limit ..."
        // содержит подстроку "unknown function", поэтому первая (более длинная)
        // должна замениться до второй, чтобы не превратиться в кашу.
        /**
         * Одна запись таблицы соответствий: подстрока LIL и пара
         * en/ru-переводов, которой её заменить.
         */
        struct rule
        {
            const char        *needle;      ///< Подстрока, как её выдаёт LIL.
            tim::translations  replacement; ///< Локализованные варианты замены.
        };
        static const rule RULES[] =
        {
            { "Too many recursive calls",
              { "Too many recursive calls"_en,
                "Слишком много рекурсивных вызовов"_ru } },
            { "catcher limit reached while trying to call unknown function",
              { "Catcher limit reached while trying to call unknown command"_en,
                "Достигнут предел ловушек при попытке вызова неизвестной команды"_ru } },
            { "unknown function",
              { "Unknown command"_en,
                "Неизвестная команда"_ru } },
            { "division by zero in expression",
              { "Division by zero in expression"_en,
                "Деление на ноль в выражении"_ru } },
            { "mixing invalid types in expression",
              { "Mixing invalid types in expression"_en,
                "Смешение недопустимых типов в выражении"_ru } },
            { "expression syntax error",
              { "Expression syntax error"_en,
                "Синтаксическая ошибка в выражении"_ru } },
        };

        bool translated = false;
        for (const rule &r: RULES)
            if (_d->_error_msg.find(r.needle) != std::string::npos)
            {
                tim::replace(_d->_error_msg, r.needle,
                             tim::translator::translate(r.replacement, __FILE__, __LINE__));
                translated = true;
            }

        // Сообщение от lil_set_error в нашем коде непереведённым через RULES
        // не пройдёт (там уже подставлен TIM_TR), но непокрытый LIL-вывод
        // мы хотим заметить и добавить новое правило в RULES.
        if (!translated)
            TIM_TRACE(warning, "Untranslated LIL error: '%s'", _d->_error_msg.c_str());

        // Конечная точка добавляется только если её ещё нет: сообщение от
        // lil_set_error может уже её содержать (а сообщения от LIL — нет).
        const char last = _d->_error_msg.back();
        if (last != '.' && last != '!' && last != '?')
            _d->_error_msg += '.';
    }

    if (ok
            && res)
        *res = lil_to_string(rv);

    lil_free_value(rv);
    lil_set_data(_d->_lil, old_data);

    _d->_evaluating = false;

    return ok;
}

/** Прерывает текущий eval через lil_break_run. */
void tim::tcl::break_eval()
{
    lil_break_run(_d->_lil, true);
}

/** \return Префикс приглашения шелла. */
const std::string &tim::tcl::prompt() const
{
    return _d->_prompt;
}

/** \return Текст последней ошибки в выбранной локали. */
const std::string &tim::tcl::error_msg() const
{
    return _d->_error_msg;
}

/** \return Позиция ошибки в исходнике (UTF8-aware). */
std::size_t tim::tcl::error_pos() const
{
    return _d->_error_pos;
}

/**
 * Возвращает список имён LIL-функций, начинающихся с prefix. Использует
 * lil_names для генерации списка.
 */
std::vector<std::string> tim::tcl::complete(const std::string &prefix) const
{
    if (prefix.empty())
        return {};

    std::vector<std::string> res;

    lil_value_t r = lil_names(_d->_lil, prefix.c_str());
    lil_list_t names = lil_subst_to_list(_d->_lil, r);
    lil_free_value(r);
    for (std::size_t i = 0; i < lil_list_size(names); ++i)
        res.emplace_back(lil_to_string(lil_list_get(names, i)));
    lil_free_list(names);
    return res;
}

/** \return Пустое множество — у LIL нет зарезервированных ключевых слов. */
std::unordered_set<std::string> tim::tcl::keywords() const
{
    return {};
}

/** \return Множество всех зарегистрированных LIL-функций. */
std::unordered_set<std::string> tim::tcl::functions() const
{
    std::unordered_set<std::string> funcs;

    lil_value_t r = lil_names(_d->_lil, nullptr);
    lil_list_t names = lil_subst_to_list(_d->_lil, r);
    lil_free_value(r);

    for (std::size_t i = 0; i < lil_list_size(names); ++i)
        funcs.emplace(lil_to_string(lil_list_get(names, i)));

    lil_free_list(names);

    return funcs;
}

// Private

/**
 * LIL-callback WRITE: получает байты, выданные `puts`/выражениями,
 * и пишет их в терминал через активный протокол.
 */
void tim::p::tcl::write(lil_t lil, const char *msg)
{
    assert(lil);

    if (!msg
            || !*msg)
        return;

    tim::tcl *self = (tim::tcl *)lil_get_data(lil);
    assert(self);

    self->terminal()->protocol()->write_str(msg);
}

/**
 * LIL-callback DISPATCH: вызывается между Tcl-операторами, делегирует
 * на зарегистрированный _dispatch_handler. Если он не задан, скрипт
 * "крутится в себе" — внешние event-loop-ы не получают слотов.
 */
void tim::p::tcl::dispatch(lil_t lil)
{
    // Во время eval lil_get_data возвращает tim::tcl* (выставляется
    // в tim::tcl::eval). Зовём зарегистрированный обработчик; если
    // его нет — просто ничего не делаем, бесконечный скрипт просто
    // не будет давать дышать внешним event-loop-ам.
    tim::tcl *self = (tim::tcl *)lil_get_data(lil);
    if (!self)
        return;
    if (self->_d->_dispatch_handler)
        self->_d->_dispatch_handler();
}
