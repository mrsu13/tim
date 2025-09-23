#pragma once

#include "tim_severity.h"

#include <cstdarg>
#include <cstddef>


#define TIM_TRACE(svrt, ...) \
    tim::tracef(tim::severity::svrt, __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)


namespace tim
{

bool vtracef(tim::severity severity,
             const char *file_name, std::size_t line,
             const char *function,
             const char *format, va_list args);

bool tracef(tim::severity severity,
            const char *file_name, std::size_t line,
            const char *function,
            const char *format, ...)
    __attribute__ ((format(printf, 5, 6)));

}
