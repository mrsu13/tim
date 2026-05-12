#include "tim_a_terminal.h"

#include "tim_a_terminal_p.h"

#include "tim_a_protocol.h"
#include "tim_string_tools.h"
#include "tim_trace.h"

#include <cassert>


// Public

/** Деструктор по умолчанию. */
tim::a_terminal::~a_terminal() = default;

/** \return Указатель на нижележащий протокол. */
tim::a_protocol *tim::a_terminal::protocol() const
{
    return _d->_proto;
}

/** \return Текущая тема терминала. */
const tim::terminal_theme &tim::a_terminal::theme() const
{
    return _d->_theme;
}

/**
 * Заменяет тему. Уже выведенный текст не перерисовывается —
 * новая тема применяется к последующим вызовам printf/cprintf.
 */
void tim::a_terminal::set_theme(const tim::terminal_theme &theme)
{
    _d->_theme = theme;
}

/**
 * va_list-вариант printf. Форматирует через tim::vsprintf и шлёт
 * результат в протокол.
 */
int tim::a_terminal::vprintf(const char *format, va_list args)
{
    assert(format && *format);

    std::string s;

    va_list args_copy;
    va_copy(args_copy, args);
    const int n = tim::vsprintf(s, format, args);
    va_end(args_copy);

    if (n > 0)
        _d->_proto->write(s.c_str(), n);

    return n;
}

/**
 * Variadic-обёртка над vprintf().
 */
int tim::a_terminal::printf(const char *format, ... )
{
    assert(format && *format);

    va_list args;
    va_start(args, format);
    const int n = tim::a_terminal::vprintf(format, args);
    va_end(args);

    return n;
}

/**
 * Печатает форматированный текст указанными цветами текста и фона;
 * после печати сбрасывает атрибуты, если хотя бы один из цветов
 * не был пустым (transparent).
 */
int tim::a_terminal::cprintf(const tim::color &text_color,
                             const tim::color &bg_color,
                             const char *format, ... )
{
    assert(format && *format);

    set_color(text_color);
    set_bg_color(bg_color);

    va_list args;
    va_start(args, format);
    const int n = tim::a_terminal::vprintf(format, args);
    va_end(args);

    if (!text_color.empty()
            || !bg_color.empty())
        reset_colors();

    return n;
}


// Protected

/**
 * Конструктор для наследников: запоминает указатель на протокол.
 */
tim::a_terminal::a_terminal(tim::a_protocol *proto)
    : _d(new tim::p::a_terminal())
{
    assert(proto);

    _d->_proto = proto;
}
