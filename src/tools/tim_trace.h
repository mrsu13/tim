#pragma once

#include "tim_severity.h"

#include <cstdarg>
#include <cstddef>


/**
 * Имя текущей функции для подстановки в TIM_TRACE. У GCC и Clang
 * есть нестандартное __PRETTY_FUNCTION__ с полной сигнатурой; на
 * прочих компиляторах используется стандартный __func__ (без типа
 * возврата и аргументов).
 */
#if defined(__GNUC__) || defined(__clang__)
#  define TIM_TRACE_FUNCTION __PRETTY_FUNCTION__
#else
#  define TIM_TRACE_FUNCTION __func__
#endif


/**
 * Атрибут проверки printf-подобного формата компилятором. GCC и Clang
 * понимают __attribute__((format(printf,...))). На MSVC оставлен
 * пустым — Microsoft использует SAL-аннотации, добавление которых
 * выходит за рамки этого заголовка.
 */
#if defined(__GNUC__) || defined(__clang__)
#  define TIM_TRACE_PRINTF_ATTR(fmt_idx, args_idx) \
        __attribute__ ((format(printf, fmt_idx, args_idx)))
#else
#  define TIM_TRACE_PRINTF_ATTR(fmt_idx, args_idx)
#endif


/**
 * Сахар над tim::tracef: подставляет __FILE__, __LINE__,
 * TIM_TRACE_FUNCTION (см. выше).
 *
 * \param svrt Не-полностью-квалифицированный элемент tim::severity
 *             (debug, info, warning, error, fatal, trace).
 * \param ... printf-подобный формат и аргументы.
 */
#define TIM_TRACE(svrt, ...) \
    tim::tracef(tim::severity::svrt, __FILE__, __LINE__, TIM_TRACE_FUNCTION, ##__VA_ARGS__)


namespace tim
{

/**
 * Вариант trace, принимающий va_list. Используется внутри tracef
 * и пригоден для собственных обёрток.
 *
 * Формат вывода (вдохновлённый rxi/log.c):
 * \code
 * HH:MM:SS LEVEL file:line: message
 * \endcode
 * При наличии TTY на stderr — с ANSI-окраской уровня и file:line.
 *
 * \param severity Уровень важности.
 * \param file_name Имя файла, обычно __FILE__.
 * \param line Номер строки, обычно __LINE__.
 * \param function Имя функции; параметр оставлен ради совместимости
 *                 API, но в выводе rxi-стиля не используется.
 * \param format printf-формат.
 * \param args Список аргументов.
 * \return true для строгих ошибок (severity > error), иначе false.
 *         Fatal завершает процесс через std::abort().
 */
bool vtracef(tim::severity severity,
             const char *file_name, std::size_t line,
             const char *function,
             const char *format, va_list args);

/**
 * Печатает сообщение в stderr в rxi-стиле (см. vtracef).
 *
 * \param severity Уровень важности.
 * \param file_name Имя файла, обычно __FILE__.
 * \param line Номер строки, обычно __LINE__.
 * \param function Имя функции (не используется в текущем формате).
 * \param format printf-формат.
 * \param ... Аргументы формата.
 * \return см. vtracef().
 */
bool tracef(tim::severity severity,
            const char *file_name, std::size_t line,
            const char *function,
            const char *format, ...)
    TIM_TRACE_PRINTF_ATTR(5, 6);

}
