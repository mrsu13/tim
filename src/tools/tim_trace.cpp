#include "tim_trace.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>

#ifdef TIM_OS_LINUX
#  include <unistd.h>
#endif

#ifdef TIM_OS_WINDOWS
#  include <io.h>
#  include <windows.h>
   // На SDK старше Windows 10 константа может отсутствовать; объявим
   // её сами, чтобы код компилировался — SetConsoleMode() для старых
   // ОС вернёт ошибку, и цвета отключатся.
#  ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#    define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#  endif
#endif


namespace tim
{

/**
 * Проверяет, что stderr указывает на терминал. Используется для
 * решения, выдавать ли ANSI ESC-последовательности цветов: при выводе
 * в файл/пайп цвета только засоряют лог.
 *
 * static-видимость ограничивает символ файлом — это файл-локальный
 * помощник, который не должен утечь в линк.
 */
static bool stderr_is_tty()
{
#ifdef TIM_OS_LINUX
    return isatty(fileno(stderr)) != 0;
#elif defined(TIM_OS_WINDOWS)
    return _isatty(_fileno(stderr)) != 0;
#else
    return false;
#endif
}

/**
 * Один раз инициализирует поддержку ANSI-цветов и возвращает её
 * актуальный статус. На Linux достаточно проверки TTY; на Windows
 * дополнительно включается ENABLE_VIRTUAL_TERMINAL_PROCESSING
 * (Windows 10+) — без него ANSI ESC-последовательности выводятся
 * как нечитаемые символы.
 *
 * \return true, если цвета можно использовать.
 */
static bool colors_enabled()
{
    static bool initialized = false;
    static bool enabled = false;
    if (initialized)
        return enabled;
    initialized = true;
    enabled = tim::stderr_is_tty();
#ifdef TIM_OS_WINDOWS
    if (enabled)
    {
        HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
        DWORD mode = 0;
        if (h == INVALID_HANDLE_VALUE
                || !GetConsoleMode(h, &mode)
                || !SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            enabled = false;
    }
#endif
    return enabled;
}

/**
 * ANSI SGR ESC-последовательность цвета для уровня. Палитра как
 * в rxi/log.c: fatal — пурпурный, error — красный, warning — жёлтый,
 * info — зелёный, debug — голубой, trace — ярко-синий.
 *
 * \return Строка-литерал, начинающаяся с ESC; либо "" для неизвестного уровня.
 */
static const char *level_color(tim::severity s)
{
    switch (s)
    {
        case tim::severity::fatal:   return "\x1b[35m";
        case tim::severity::error:   return "\x1b[31m";
        case tim::severity::warning: return "\x1b[33m";
        case tim::severity::info:    return "\x1b[32m";
        case tim::severity::debug:   return "\x1b[36m";
        case tim::severity::trace:   return "\x1b[94m";
    }
    return "";
}

/**
 * Пишет в \a out строку текущего локального времени в формате
 * HH:MM:SS. Использует thread-safe localtime_r на Linux и
 * localtime_s на Windows.
 *
 * \param out Буфер.
 * \param out_size Размер буфера (минимум 9 байт).
 */
static void write_timestamp(char *out, std::size_t out_size)
{
    std::time_t now = std::time(nullptr);
    std::tm tm_buf;
#ifdef TIM_OS_WINDOWS
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    std::strftime(out, out_size, "%H:%M:%S", &tm_buf);
}

}


/**
 * Печатает строку в stderr в формате "HH:MM:SS LEVEL file:line: msg",
 * с ANSI-окраской уровня и серым file:line при TTY-выводе.
 * Fatal завершает процесс через std::abort().
 *
 * \return true для строгих ошибок (severity > error); fatal не возвращает.
 */
bool tim::vtracef(tim::severity severity,
                 const char *file_name, std::size_t line,
                 const char *function,
                 const char *format, va_list args)
{
    // Параметр function оставлен в API ради совместимости с прежней
    // подписью TIM_TRACE — в rxi-стиле формата он не используется.
    (void) function;

    char ts[16];
    tim::write_timestamp(ts, sizeof(ts));

    if (tim::colors_enabled())
        std::fprintf(stderr, "%s %s%s\x1b[0m \x1b[90m%s:%zu:\x1b[0m ",
                     ts,
                     tim::level_color(severity),
                     tim::severity_title(severity),
                     file_name, line);
    else
        std::fprintf(stderr, "%s %s %s:%zu: ",
                     ts,
                     tim::severity_title(severity),
                     file_name, line);

    va_list args_copy;
    va_copy(args_copy, args);
    std::vfprintf(stderr, format, args_copy);
    va_end(args_copy);
    std::fprintf(stderr, "\n");

    if (severity == tim::severity::fatal)
        std::abort();

    return severity > tim::severity::error;
}

/**
 * Variadic-обёртка над vtracef(). Используется TIM_TRACE-макросом.
 */
bool tim::tracef(tim::severity severity,
                 const char *file_name, std::size_t line,
                 const char *function,
                 const char *format, ...)
{
    va_list args;
    va_start(args, format);
    const bool res = tim::vtracef(severity, file_name, line, function, format, args);
    va_end(args);
    return res;
}
