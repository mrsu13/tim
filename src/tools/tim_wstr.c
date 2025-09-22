#include "tim_wstr.h"

#include "tim_wstring.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


typedef struct tim_wstr
{
    wchar_t *buf;
    size_t len;
} tim_wstr_t;


// Public

tim_wstr_t *tim_wstr_new(const wchar_t *raw_s)
{
    tim_wstr_t *s = calloc(1, sizeof(tim_wstr_t));
    assert(s && "Failed to allocate memory for Wide String.");

    if (raw_s)
    {
        s->buf = wcsdup(raw_s);
        s->len = wcslen(s->buf);
    }

    return s;
}

void tim_wstr_free(tim_wstr_t *s)
{
    assert(s);

    free(s->buf);
    free(s);
}

const wchar_t *tim_wcstr(const tim_wstr_t *s)
{
    assert(s);
    return s->buf;
}

size_t tim_wcslen(const tim_wstr_t *s)
{
    assert(s);
    return s->len;
}

const wchar_t *tim_wcsncat(tim_wstr_t *dst, const wchar_t *src, size_t n)
{
    assert(dst);
    assert(src);

    if (*src
            && n)
    {
        wchar_t *new_buf = realloc(dst->buf, (dst->len + n) * sizeof(wchar_t));
        assert(new_buf && "Failed to reallocate memory for wide string.");

        wmemcpy(new_buf + dst->len, src, n);
        dst->buf = new_buf;
        dst->len += n;
    }

    return dst->buf;
}

const wchar_t *tim_wcscat(tim_wstr_t *dst, const wchar_t *src)
{
    return tim_wcsncat(dst, src, wcslen(src));
}

const wchar_t *tim_wcscat_fill(tim_wstr_t *dst, wchar_t fill, size_t n)
{
    assert(dst);

    if (n)
    {
        wchar_t *new_buf = realloc(dst->buf, (dst->len + n) * sizeof(wchar_t));
        assert(new_buf && "Failed to reallocate memory for Wide String.");

        wmemset(new_buf + dst->len, fill, n);
        dst->buf = new_buf;
        dst->len += n;
    }

    return dst->buf;
}

const wchar_t *tim_wcscat_swprintf(tim_wstr_t *dst, const wchar_t *format, ...)
{
    assert(dst);
    assert(format && *format);

    wchar_t *s = NULL;

    va_list args;
    va_start(args, format);
    const int n = tim_vswprintf(&s, format, args);
    va_end(args);

    if (n < 0)
        return NULL;

    tim_wcsncat(dst, s, n);
    free(s);

    return dst->buf;
}
