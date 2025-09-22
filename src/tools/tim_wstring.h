#pragma once

#include <stdarg.h>
#include <wchar.h>


int tim_swprintf(wchar_t **dst, const wchar_t *format, ...)
                 /* __attribute__ ((format(wprintf, 2, 3))) */;

int tim_vswprintf(wchar_t **dst, const wchar_t *format, va_list args);

size_t tim_to_ws(wchar_t **dst, const char *src);
size_t tim_from_ws(char **dst, const wchar_t *src);
