#include "tim_trace.h"

#include <stdio.h>
#include <stdlib.h>


void tim_vtracef(tim_severity_t severity,
                 const char *file_name, size_t line,
                 const char *function,
                 const char *format, va_list args)
{
#ifdef TIM_DEBUG
    fprintf(stderr, "%s %s:%d %s: ",
            tim_severity_title(severity),
            file_name,
            line,
            function);
#else
    (void) file_name;
    (void) line;
    (void) function;

    fprintf(stderr, "%s: ", tim_severity_title(severity));
#endif

    va_list args_copy;
    va_copy(args_copy, args);
    vfprintf(stderr, format, args_copy);
    va_end(args_copy);
    fprintf(stderr, "%s", "\n");
}

void tim_tracef(tim_severity_t severity,
                const char *file_name, size_t line,
                const char *function,
                const char *format, ...)
{
    va_list args;
    va_start(args, format);
    tim_vtracef(severity, file_name, line, function, format, args);
    va_end(args);
}
