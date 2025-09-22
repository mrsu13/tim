#include "tim_wstring.h"

#include "tim_trace.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


int tim_swprintf(wchar_t **dst, const wchar_t *format, ...)
{
    assert(dst);
    assert(format && *format);

    va_list args;

    va_start(args, format);
    const int n = tim_vswprintf(dst, format, args);
    va_end(args);

    return n;
}

int tim_vswprintf(wchar_t **dst, const wchar_t *format, va_list args)
{
    assert(dst);
    assert(format && *format);

    static const size_t SIZE_LIMIT = 10 * 1024; // 10kB

    assert(format);
    size_t size = 256;
    *dst = (wchar_t *)malloc(size * sizeof(wchar_t));
    assert(*dst);

    int n = 0;
    while (true)
    {
        va_list args_copy;
        va_copy(args_copy, args);
        n = vswprintf(*dst, size, format, args_copy);
        va_end(args_copy);
        if (n < (int)size)
            break;

        if ((size *= 2) > SIZE_LIMIT)
        {
            free(*dst);
            *dst = NULL;

            TIM_TRACE(Error, "%s",
                      "Memory size limit exceeded in tim_vswprintf().");

            return -1;
        }
        wchar_t *new_buffer = (wchar_t *)realloc(*dst, size);
        if (!new_buffer)
        {
            free(*dst);
            *dst = NULL;

            TIM_TRACE(Error, "%s",
                      "Failed to reallocate memory in tim_vswprintf().");

            return -1;
        }
        *dst = new_buffer;
    }

    return n;
}

size_t tim_to_ws(wchar_t **dst, const char *src)
{
    assert(dst);
    assert(src);

    const size_t len = strlen(src); // The maximum.
    *dst = malloc((len + 1) * sizeof(wchar_t));
    assert(*dst && "Failed to allocate memory for a wide string.");

    return mbstowcs(*dst, src, len);
}

size_t tim_from_ws(char **dst, const wchar_t *src)
{
    assert(dst);
    assert(src);

    const size_t len = wcslen(src) * 4; // The maximum.
    *dst = malloc(len + 1);
    assert(*dst && "Failed to allocate memory for a string.");

    return wcstombs(*dst, src, len);
}
