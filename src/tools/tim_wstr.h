#pragma once

#include <stddef.h>


typedef struct tim_wstr tim_wstr_t;

tim_wstr_t *tim_wstr_new(const wchar_t *raw_s);
void tim_wstr_free(tim_wstr_t *s);

const wchar_t *tim_wcstr(const tim_wstr_t *s);
size_t tim_wcslen(const tim_wstr_t *s);
const wchar_t *tim_wcsncat(tim_wstr_t *dst, const wchar_t *src, size_t n);
const wchar_t *tim_wcscat(tim_wstr_t *dst, const wchar_t *src);
const wchar_t *tim_wcscat_fill(tim_wstr_t *dst, wchar_t fill, size_t n);
const wchar_t *tim_wcscat_swprintf(tim_wstr_t *dst, const wchar_t *format, ...);
