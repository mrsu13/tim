#include "tim_string.h"

#include "tim_trace.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


char *tim_sprintf(const char *format, ...)
{
    assert(format);

    va_list args;

    va_start(args, format);
    char *s = tim_vsprintf(format, args);
    va_end(args);

    return s;
}

char *tim_vsprintf(const char *format, va_list args)
{
    static const size_t SIZE_LIMIT = 10 * 1024; // 10kB

    assert(format);
    size_t size = 256;
    char *buffer = (char *)malloc(size);
    assert(buffer);

    int n = 0;
    while (true)
    {
        va_list args_copy;
        va_copy(args_copy, args);
        n = vsnprintf(buffer, size, format, args_copy);
        va_end(args_copy);
        if (n < (int)size)
            break;

        if ((size *= 2) > SIZE_LIMIT)
        {
            free(buffer);

            TIM_TRACE(Error, "%s",
                      "Memory size limit exceeded in tim_vsprintf().");

            return NULL;
        }
        char *new_buffer = (char *)realloc(buffer, size);
        if (!new_buffer)
        {
            free(buffer);

            TIM_TRACE(Error, "%s",
                      "Failed to reallocate memory in tim_vsprintf().");

            return NULL;
        }
        buffer = new_buffer;
    }

    return buffer;
}
