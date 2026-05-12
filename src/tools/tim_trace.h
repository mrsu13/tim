#pragma once

#include "tim_severity.h"

#include <cstdarg>
#include <cstddef>


/**
 * Сахар над tim::tracef: подставляет __FILE__, __LINE__, __PRETTY_FUNCTION__.
 *
 * \param svrt Не-полностью-квалифицированный элемент tim::severity
 *             (debug, info, warning, error, fatal, trace).
 * \param ... printf-подобный формат и аргументы.
 */
#define TIM_TRACE(svrt, ...) \
    tim::tracef(tim::severity::svrt, __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)


namespace tim
{

/**
 * Варианты trace, принимающие va_list. Используется внутри tracef
 * и пригоден для собственных обёрток.
 *
 * \param severity Уровень важности.
 * \param file_name Имя файла, обычно __FILE__.
 * \param line Номер строки, обычно __LINE__.
 * \param function Имя функции, обычно __PRETTY_FUNCTION__.
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
 * Печатает сообщение в stderr с префиксом уровня и местоположения.
 *
 * \param severity Уровень важности.
 * \param file_name Имя файла, обычно __FILE__.
 * \param line Номер строки, обычно __LINE__.
 * \param function Имя функции, обычно __PRETTY_FUNCTION__.
 * \param format printf-формат.
 * \param ... Аргументы формата.
 * \return см. vtracef().
 */
bool tracef(tim::severity severity,
            const char *file_name, std::size_t line,
            const char *function,
            const char *format, ...)
    __attribute__ ((format(printf, 5, 6)));

}
