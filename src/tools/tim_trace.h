#pragma once

#include "tim_severity.h"

#include <stdarg.h>
#include <stddef.h>


#define TIM_TRACE(severity, ...) \
    tim_tracef(Tim##severity, __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)


void tim_vtracef(tim_severity_t severity,
                 const char *file_name, size_t line,
                 const char *function,
                 const char *format, va_list args);

void tim_tracef(tim_severity_t severity,
                const char *file_name, size_t line,
                const char *function,
                const char *format, ...)
                __attribute__ ((format(printf, 5, 6)));
