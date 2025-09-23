#include "tim_string_tools.h"

#include "tim_string_vector.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <locale>
#include <regex>


std::wstring tim::to_wstring(const std::string &s)
{
    const std::size_t len = std::mbstowcs(nullptr, s.c_str(), 0) + 1;
    wchar_t *buffer = new wchar_t[len];
    std::mbstowcs(buffer, s.c_str(), len);
    std::wstring ws(buffer);
    delete[] buffer;
    return ws;
}

std::string tim::from_wstring(const std::wstring &ws)
{
    const std::size_t len = std::wcstombs(nullptr, ws.c_str(), 0) + 1;
    char *buffer = new char[len];
    std::wcstombs(buffer, ws.c_str(), len);
    std::string s(buffer);
    delete[] buffer;
    return s;
}

std::string tim::escape_quotes(const std::string &str)
{
    std::string esc_str;
    esc_str.reserve(str.size());

    for (const char c: str)
        if (c == '"')
            esc_str += "\\\"";
        else
            esc_str += c;

    return esc_str;
}

std::string tim::parse_unicode(const std::string &str)
{
    const nlohmann::json j = nlohmann::json::parse("\"" + tim::escape_quotes(str) + "\"",
                                                   nullptr, false, true);
    return j.is_discarded()
                ? str
                : j.get<std::string>();
}

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

std::string tim::to_lower(const std::string &s)
{
    std::wstring ws = tim::to_wstring(s);
    std::transform(ws.cbegin(), ws.cend(), ws.begin(),
                   [](std::wstring::value_type c)
                   {
                       return std::towlower(c);
                   });
    return tim::from_wstring(ws);
}

std::string tim::to_upper(const std::string &s)
{
    std::wstring ws = tim::to_wstring(s);
    std::transform(ws.cbegin(), ws.cend(), ws.begin(),
                   [](std::wstring::value_type c)
                   {
                       return std::towupper(c);
                   });
    return tim::from_wstring(ws);
}

std::string tim::to_capitalized(const std::string &s)
{
    if (s.empty())
        return {};

    std::wstring cs = tim::to_wstring(s);
    bool space = true;
    for (wchar_t &c: cs)
    {
        if (std::iswspace(c))
            space = true;
        else if (space)
        {
            c = std::towupper(c);
            space = false;
        }
    }

    return tim::from_wstring(cs);
}

bool tim::starts_with(const std::string &str,
                      const std::string &prefix,
                      const tim::cs cs)
{
    if (prefix.length() > str.length())
        return false;
    if (cs == tim::cs::Sensitive)
        return std::strncmp(str.c_str(), prefix.c_str(), std::min(str.length(), prefix.length())) == 0;

    return std::equal(prefix.cbegin(), prefix.cend(),
                      str.cbegin(),
                      [](char a, char b)
                      {
                            return std::tolower(a) == std::tolower(b);
                      });
}

bool tim::ends_with(const std::string &str,
                    const std::string &suffix,
                    const tim::cs cs)
{
    return str.length() < suffix.length()
                ? false
                : tim::equal(str.substr(str.length() - suffix.length()), suffix, cs);
}

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

std::string tim::trim_left(const std::string &s, const char *delimiters)
{
    assert(delimiters && *delimiters);

    std::string::size_type st = s.find_first_not_of(delimiters);
    if (st == std::string::npos)
        return std::string();
    return s.substr(st);
}

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

std::string tim::simplify(const std::string &s)
{
    if (s.empty())
        return s;

    const std::string::value_type *src = s.data();
    const std::string::value_type *end = s.data() + s.size();
    std::string result = s;

    std::string::value_type *dst = result.data();
    std::string::value_type *ptr = dst;
    bool unmodified = true;
    while (true)
    {
        while (src != end
                    && std::isspace(*src))
            ++src;
        while (src != end
                && !std::isspace(*src))
            *ptr++ = *src++;
        if (src == end)
            break;
        if (*src != ' ')
            unmodified = false;
        *ptr++ = ' ';
    }
    if (ptr != dst
            && ptr[-1] == ' ')
        --ptr;

    const std::size_t newlen = ptr - dst;
    if (newlen == s.size()
            && unmodified)
        return s;

    result.resize(newlen);

    return result;
}

void tim::wrap(std::string &s, std::size_t pos)
{
    if (pos)
        for (std::size_t i = pos; i < s.length(); i += pos)
            s.insert(i++, 1, '\n');
}

std::string tim::unwrap(const std::string &s)
{
    std::string line(s.size(), ' ');

    std::size_t i = 0;
    for (const std::string::value_type &c: s)
        line[i++] = (c == '\n'
                            || c == '\r')
                        ? ' '
                        : c;
    return line;
}

std::string tim::strip_pem(const std::string &pem)
{
    static const char *const RE_PEM_HEADER = R"(\-{5}[\w\s]+\-{5})";

    std::string s = tim::trim(std::regex_replace(pem, std::regex(RE_PEM_HEADER, std::regex::ECMAScript), ""));
    return tim::unwrap(s);
}

int tim::vsprintf(std::string &s, const char *format, va_list args)
{
    assert(format && *format);

    static const std::size_t SIZE_LIMIT = 10 * 1024 * 1024; // 10MB

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

            TIM_TRACE(Error,
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

std::string tim::vsprintf(const char *format, va_list args)
{
    std::string s;
    va_list args_copy;
    va_copy(args_copy, args);
    tim::vsprintf(s, format, args_copy);
    va_end(args_copy);
    return s;
}

int tim::sprintf(std::string &s, const char *format, ...)
{
    assert(format && *format);

    va_list args;

    va_start(args, format);
    const int n = tim::vsprintf(s, format, args);
    va_end(args);

    return n;
}

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
 * \param c Hex digit. Both upper and lower case characters are supported.
 * \return Decimal number.
 */
char tim::from_hex(char c)
{
    return (c >= '0' && c <= '9')
                ? c - '0'
                : ((c >= 'A' && c <= 'F')
                       ? c - 'A' + 10
                       : c - 'a' + 10);
}

/**
 * Convert vector of bytes to string containing its hex representation.
 */
std::string tim::to_string(const tim::byte_vector &data)
{
    static const char *const DIGITS = "0123456789abcdef";

    if (data.empty())
        return {};

    std::string s;
    s.reserve(data.size() * 2);
    for (std::uint8_t c: data)
    {
        s += DIGITS[(c >> 4) & 0xF];
        s += DIGITS[c & 0xF];
    }

    return s;
}

/**
 * Convert string containing hex representation to a vector of bytes.
 *
 * \param s Hex representation.
 * \param ok \c true if succeeded, and \c false --- if failed.
 * \return Vector of bytes if succeeded, and empty vector if failed.
 */
tim::byte_vector tim::from_string(const std::string &s, bool *ok)
{
    if (ok)
        *ok = true;

    if (s.empty())
        return {};

    if (s.length() % 2)
    {
        TIM_TRACE(Error,
                  TIM_TR("Invalid hex string '%s'."_en,
                         "Некорректная шестнадцатеричная строка '%s'."_ru),
                  s.c_str());
        if (ok)
            *ok = false;
        return {};
    }

    tim::byte_vector data;
    data.reserve(s.size() / 2);
    std::string::const_iterator i = s.cbegin();
    std::string::const_iterator e = s.cend();
    for (; i != e; ++i)
        data.emplace_back((from_hex(*i++) << 4) + from_hex(*i));

    return data;
}

bool tim::from_string(const std::string &json, nlohmann::json &j)
{
    if (json.empty())
    {
        j = nlohmann::json();
        return true;
    }

    j = nlohmann::json::parse(json,
                              nullptr, // Parser call-back.
                              false, // Allow exceptions.
                              true); // Ignore comments.

    if (j.is_discarded())
        return TIM_TRACE(Error,
                        TIM_TR("Failed to parse JSON at position %s-%s."_en,
                              "Ошибка при разборе JSON в позиции %s-%s."_ru),
                        j.start_pos() == std::string::npos
                            ? tim::na()
                            : std::to_string(j.start_pos()).c_str(),
                        j.end_pos() == std::string::npos
                            ? tim::na()
                            : std::to_string(j.end_pos()).c_str());

    return true;
}

bool tim::to_bool(const std::string &s, bool *ok)
{
    if (s.empty()
            || tim::strcasecmp(s.c_str(), "false")
            || tim::strcasecmp(s.c_str(), "0")
            || tim::strcasecmp(s.c_str(), "no")
            || tim::strcasecmp(s.c_str(), "off"))
    {
        if (ok)
            *ok = true;
        return false;
    }

    if (tim::strcasecmp(s.c_str(), "true")
            || tim::strcasecmp(s.c_str(), "1")
            || tim::strcasecmp(s.c_str(), "yes")
            || tim::strcasecmp(s.c_str(), "on"))
    {
        if (ok)
            *ok = true;
        return true;
    }

    if (ok)
        *ok = false;
    return false;
}

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

unsigned tim::to_uint(const std::string &s, bool *ok)
{
    if (s.empty())
        return 0;

    char *e;
    const unsigned i = std::strtoul(s.c_str(), &e, 0);
    if (ok)
        *ok = ! *e;
    return i;
}

long long tim::to_long_long(const std::string &s, bool *ok)
{
    if (s.empty())
        return 0;

    char *e;
    const long long i = std::strtoll(s.c_str(), &e, 0);
    if (ok)
        *ok = ! *e;
    return i;
}

unsigned long long tim::to_ulong_long(const std::string &s, bool *ok)
{
    if (s.empty())
        return 0;

    char *e;
    const unsigned long long i = std::strtoull(s.c_str(), &e, 0);
    if (ok)
        *ok = ! *e;
    return i;
}

float tim::to_float(const std::string &s, bool *ok)
{
    if (s.empty())
        return 0.0f;

    char *e;
    const float f = std::strtof(s.c_str(), &e);
    if (ok)
        *ok = ! *e;
    return f;
}

double tim::to_double(const std::string &s, bool *ok)
{
    if (s.empty())
        return 0.0;

    char *e;
    const double d = std::strtod(s.c_str(), &e);
    if (ok)
        *ok = ! *e;
    return d;
}

std::string tim::member_name(const char *name)
{
    assert(name && *name);

    const char *c = name;
    const char *n = name;
    while (*c)
    {
        if (*c == ':')
            n = c + 1;
        ++c;
    }
    return std::string(n);
}

/** Constructs a dotted name on the base of its
    name in camel or snake case.

    \param snake Name in camel or snake case.
    \return Name with dots.
*/
std::string tim::to_dotted(const std::string &snake)
{
    if (snake.empty())
        return "";

    std::string dotted;
    bool prev_upper = std::isupper(snake[0]); // If the first letter in property name is capital,
                                              // do not add a leading dot.
    for (const std::string::value_type c: snake)
    {
        if (!prev_upper
                && (c == '_'
                        || std::isupper(c)))
        {
            prev_upper = true;
            dotted += '.';
            if (c == '_')
                continue;
        }
        else
            prev_upper = false;

        dotted += std::tolower(c);
    }
    return dotted;
}

std::string tim::to_snake(const std::string &dotted)
{
    std::string snake(dotted);
    std::replace(snake.begin(), snake.end(), '.', '_');
    return snake;
}

std::string tim::centered(const std::string &str, std::size_t width)
{
    const int l = static_cast<int>(width - str.size());
    if (l <= 0)
        return str;

    const std::size_t q = l >> 1;
    const std::size_t r = l & 1;

    return std::string(q, ' ')
                + str
                + std::string(q + r, ' ');
}

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
        case tim::elide::Left:
            wstr.erase(0, str.size() - width - DOTS_LEN);
            wstr.insert(0, DOTS, DOTS_LEN);
            break;

        case tim::elide::Right:
            wstr.erase(width - DOTS_LEN);
            wstr.append(DOTS, DOTS_LEN);
            break;

        case tim::elide::Middle:
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

std::string tim::aligned(const std::string &str, tim::text_align al, std::size_t width)
{
    if (str.empty()
            || width == 0)
        return str;

    std::wstring text = tim::to_wstring(str);

    tim::wstring_vector pars;
    tim::wstring_vector words;
    std::size_t space_num, space_rem;
    std::size_t delta;
    std::wstring aligned_text;
    std::wstring space;

    pars = tim::split<tim::wstring_vector>(text, L"\n", tim::split_mode::SkipEmptyParts);
    tim::wstring_vector::const_iterator pb = pars.cbegin();
    tim::wstring_vector::const_iterator pi = pb;
    tim::wstring_vector::const_iterator pe = pars.cend();
    for (; pi != pe; ++pi)
    {
        if (pi != pb)
            aligned_text += '\n';

        words = tim::split<tim::wstring_vector>(*pi, L" \t", tim::split_mode::SkipEmptyParts);
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
                space_num = delta != 0 ? (al == tim::text_align::Justify ? (width - line_width) / delta : 1) : 0;
                space_rem = delta != 0 ? (al == tim::text_align::Justify ? (width - line_width) % delta : 0) : 0;

                if (begin != 0)
                    aligned_text += '\n';
                if (al == tim::text_align::Right)
                {
                    space.assign(width - line_width - delta, ' ');
                    aligned_text.append(space);
                }
                else if (al == tim::text_align::Center)
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
            if (al == tim::text_align::Center)
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

bool tim::strcasecmp(const char *s1, const char *s2)
{
#ifdef TIM_OS_WINDOWS
    return _stricmp(s1, s2) == 0;
#else
    return ::strcasecmp(s1, s2) == 0;
#endif
}
