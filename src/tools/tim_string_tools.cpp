#include "tim_string_tools.h"

#include "tim_string_vector.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include <cassert>
#include <codecvt>
#include <cstdlib>
#include <cwctype>
#include <locale>


/**
 * Преобразует UTF-8 строку в wide-string через codecvt_utf8.
 */
std::wstring tim::to_wstring(const std::string &s)
{
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(s);
}

/**
 * Преобразует wide-string в UTF-8 через codecvt_utf8.
 */
std::string tim::from_wstring(const std::wstring &ws)
{
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(ws);
}

/**
 * Заменяет все вхождения подстроки. Идёт линейно, сдвигая курсор на
 * длину замены — устойчиво к случаю, когда new_str содержит old_str.
 */
void tim::replace(std::string &str,
                 const std::string &old_str,
                 const std::string &new_str)
{
    std::string::size_type pos = 0;
    std::string::size_type ol = old_str.length();
    std::string::size_type nl = new_str.length();
    while ((pos = str.find(old_str, pos)) != std::string::npos)
    {
        str.replace(pos, ol, new_str);
        pos += nl;
    }
}

/**
 * Обрезает delimiters с правого конца строки. delimiters не должен
 * быть пуст (проверяется ассертом).
 */
std::string tim::trim_right(const std::string &s, const char *delimiters)
{
    assert(delimiters && *delimiters);

    std::string::size_type st = s.find_last_not_of(delimiters);
    if (!st
            || st == std::string::npos)
        return std::string();
    return s.substr(0, st + 1 >= s.length()
                ? std::string::npos
                : st + 1);
}

/**
 * Обрезает delimiters с левого конца строки.
 */
std::string tim::trim_left(const std::string &s, const char *delimiters)
{
    assert(delimiters && *delimiters);

    std::string::size_type st = s.find_first_not_of(delimiters);
    if (st == std::string::npos)
        return std::string();
    return s.substr(st);
}

/**
 * Обрезает delimiters с обоих концов строки.
 */
std::string tim::trim(const std::string &s, const char *delimiters)
{
    assert(delimiters && *delimiters);

    std::string::size_type st1 = s.find_first_not_of(delimiters);
    if (st1 == std::string::npos)
        return std::string();

    std::string::size_type st2 = s.find_last_not_of(delimiters);
    if (st2 == std::string::npos)
        return std::string();
    if (st2 < st1)
        return std::string();
    return s.substr(st1, st2 + 1 >= s.size()
                            ? std::string::npos
                            : st2 + 1 - st1);
}

/**
 * Форматирует через vsnprintf, удваивая буфер при нехватке места.
 * Ограничивает рост буфера 10MB — за пределом считает запрос ошибочным
 * и логирует error.
 */
int tim::vsprintf(std::string &s, const char *format, va_list args)
{
    assert(format && *format);

    static const std::size_t SIZE_LIMIT = 10 * 1024 * 1024; // 10 МБ

    std::size_t size = 1024;
    s.resize(size);

    int n = 0;
    while (true)
    {
        va_list args_copy;
        va_copy(args_copy, args);
        n = std::vsnprintf(&s[0], size, format, args_copy);
        va_end(args_copy);
        if (n < (int)size)
            break;

        if ((size *= 2) > SIZE_LIMIT)
        {
            s.clear();

            TIM_TRACE(error,
                      "%s",
                      TIM_TR("Memory size limit exceeded in tim::vsprintf()."_en,
                             "Превышен предел размера строки в tim::vsprintf()."_ru));

            return -1;
        }
        s.resize(size);
    }

    if (n > 0)
        s.resize(n);
    else
        s.clear();

    return n;
}

/**
 * Удобный перегруз vsprintf(): возвращает новую строку с результатом.
 */
std::string tim::vsprintf(const char *format, va_list args)
{
    std::string s;
    va_list args_copy;
    va_copy(args_copy, args);
    tim::vsprintf(s, format, args_copy);
    va_end(args_copy);
    return s;
}

/**
 * Variadic-обёртка над vsprintf(string&, format, va_list).
 */
int tim::sprintf(std::string &s, const char *format, ...)
{
    assert(format && *format);

    va_list args;

    va_start(args, format);
    const int n = tim::vsprintf(s, format, args);
    va_end(args);

    return n;
}

/**
 * Variadic-обёртка над vsprintf(format, va_list): возвращает строку.
 */
std::string tim::sprintf(const char *format, ...)
{
    assert(format && *format);

    va_list args;

    va_start(args, format);
    const std::string str = tim::vsprintf(format, args);
    va_end(args);

    return str;
}

/**
 * Парсит строку как int через strtol. Если \a ok не nullptr,
 * записывает в него true только когда вся строка распознана.
 */
int tim::to_int(const std::string &s, bool *ok)
{
    if (s.empty())
        return 0;

    char *e;
    const int i = (int)std::strtol(s.c_str(), &e, 0);
    if (ok)
        *ok = ! *e;
    return i;
}

/**
 * Усекает строку до \a width, заменяя усечённую часть на "...".
 * При width <= 3 (длина эллипсиса) возвращает сам эллипсис.
 */
std::string tim::elided(const std::string &str, std::size_t width, tim::elide el)
{
    static const wchar_t *DOTS = L"...";
    static const std::size_t DOTS_LEN = std::wcslen(DOTS);

    if (width <= DOTS_LEN)
        return tim::from_wstring(DOTS);

    std::wstring wstr = tim::to_wstring(str);

    if (wstr.size() <= width)
        return str;

    switch (el)
    {
        case tim::elide::left:
            wstr.erase(0, str.size() - width - DOTS_LEN);
            wstr.insert(0, DOTS, DOTS_LEN);
            break;

        case tim::elide::right:
            wstr.erase(width - DOTS_LEN);
            wstr.append(DOTS, DOTS_LEN);
            break;

        case tim::elide::middle:
        {
            const std::size_t d = (width - DOTS_LEN) / 2;
            wstr = wstr.substr(0, d)
                        + DOTS
                        + wstr.substr(str.size() - d);
            break;
        }
    }

    return tim::from_wstring(wstr);
}

/**
 * Выравнивает многострочный текст по ширине. Каждый абзац (разделённый
 * \\n) выравнивается отдельно: режется по словам и собирается строками
 * не шире \a width с добавлением пробелов согласно \a al.
 */
std::string tim::aligned(const std::string &str, tim::text_align al, std::size_t width)
{
    if (str.empty()
            || width == 0)
        return str;

    const std::wstring text = tim::to_wstring(str);
    if (text.size() <= width)
        return str;

    tim::wstring_vector pars;
    tim::wstring_vector words;
    std::size_t space_num, space_rem;
    std::size_t delta;
    std::wstring aligned_text;
    std::wstring space;

    pars = tim::split<tim::wstring_vector>(text, L"\n", tim::split_mode::skip_empty_parts);
    tim::wstring_vector::const_iterator pb = pars.cbegin();
    tim::wstring_vector::const_iterator pi = pb;
    tim::wstring_vector::const_iterator pe = pars.cend();
    for (; pi != pe; ++pi)
    {
        if (pi != pb)
            aligned_text += '\n';

        words = tim::split<tim::wstring_vector>(*pi, L" \t", tim::split_mode::skip_empty_parts);
        std::size_t begin = 0;
        std::size_t end = 0;
        std::size_t line_width = 0;
        for (std::size_t i = 0; i < words.size(); ++i)
        {
            std::size_t word_width = words[i].length();
            if (line_width + end - begin + word_width + 1 <= width)
            {
                line_width += word_width;
                end = i;
            }
            else
            {
                delta = end - begin;
                space_num = delta != 0 ? (al == tim::text_align::justify ? (width - line_width) / delta : 1) : 0;
                space_rem = delta != 0 ? (al == tim::text_align::justify ? (width - line_width) % delta : 0) : 0;

                if (begin != 0)
                    aligned_text += '\n';
                if (al == tim::text_align::right)
                {
                    space.assign(width - line_width - delta, ' ');
                    aligned_text.append(space);
                }
                else if (al == tim::text_align::center)
                {
                    space.assign((width - line_width - delta) >> 1, ' ');
                    aligned_text.append(space);
                }
                for (std::size_t j = begin; j <= end; ++j)
                {
                    aligned_text.append(words[j]);
                    if (j < end - 1)
                    {
                        if (space_rem != 0)
                        {
                            space.assign(space_num + 1, ' ');
                            --space_rem;
                        }
                        else
                            space.assign(space_num, ' ');
                        aligned_text.append(space);
                    }
                    else if (j == end - 1)
                    {
                        space.assign(space_num + space_rem, ' ');
                        aligned_text.append(space);
                    }
                }
                begin = end = i;
                line_width = word_width;
            }
        }

        if (line_width != 0)
        {
            if (begin != 0)
                aligned_text += '\n';
            if (al == tim::text_align::center)
            {
                space.assign((width - line_width - (end - begin)) >> 1, ' ');
                aligned_text.append(space);
            }
            for (std::size_t j = begin; j <= end; ++j)
            {
                aligned_text.append(words[j]);
                if (j <= end - 1)
                    aligned_text += ' ';
            }
        }
    }

    return tim::from_wstring(aligned_text);
}
