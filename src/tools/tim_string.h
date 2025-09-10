#pragma once

#include <stdarg.h>


#define TIM_MULTI_LINE(...) #__VA_ARGS__

char *tim_sprintf(const char *format, ...)
    __attribute__ ((format(printf, 1, 2)));

char *tim_vsprintf(const char *format, va_list args);
