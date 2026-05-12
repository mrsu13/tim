#pragma once

#include "tim_byte_vector.h"
#include "tim_elide.h"
#include "tim_endian.h"
#include "tim_json.h"
#include "tim_translator.h"
#include "tim_type_traits.h"

#include <cstdarg>
#include <list>
#include <string>
#include <vector>


namespace tim
{

/**
 * Преобразует UTF-8 std::string в wide-string (UTF-32 на Linux).
 *
 * \param s Исходная UTF-8 строка.
 * \return Wide-строка с тем же текстом.
 */
std::wstring to_wstring(const std::string &s);

/**
 * Преобразует wide-string в UTF-8.
 *
 * \param ws Исходная wide-строка.
 * \return UTF-8 строка.
 */
std::string from_wstring(const std::wstring &ws);

/**
 * Заменяет двойные кавычки в строке на экранированные (\").
 *
 * \param str Исходная строка.
 * \return Копия со всеми " → \".
 */
std::string escape_quotes(const std::string &str);

/**
 * Парсит unicode escape-последовательности (\\uXXXX) в реальные UTF-8 байты.
 *
 * \param str Строка, возможно содержащая \\uXXXX.
 * \return Декодированная строка.
 */
std::string parse_unicode(const std::string &str);

/**
 * Заменяет все вхождения \a old_str на \a new_str в \a str (in-place).
 *
 * \param str Строка, изменяемая по месту.
 * \param old_str Подстрока для поиска.
 * \param new_str Подстрока-замена.
 */
void replace(std::string &str,
             const std::string &old_str,
             const std::string &new_str);

/** Переводит строку в нижний регистр (in-place). \param s Строка. */
void to_lower(std::string &s);
/** \param s Строка. \return Копия в нижнем регистре. */
std::string to_lower(const std::string &s);

/** Переводит строку в верхний регистр (in-place). \param s Строка. */
void to_upper(std::string &s);
/** \param s Строка. \return Копия в верхнем регистре. */
std::string to_upper(const std::string &s);

/**
 * Делает первую букву заглавной, остальные — строчные.
 *
 * \param s Строка.
 * \return Копия с capitalized-видом.
 */
std::string to_capitalized(const std::string &s);

/**
 * Чувствительность к регистру для функций сравнения строк.
 */
enum class cs
{
    sensitive   = 0, ///< С учётом регистра.
    insensitive = 1  ///< Без учёта регистра.
};

/**
 * Проверка, начинается ли \a str с \a prefix.
 *
 * \param str Проверяемая строка.
 * \param prefix Искомый префикс.
 * \param cs Чувствительность к регистру.
 * \return true, если \a str начинается с \a prefix.
 */
bool starts_with(const std::string &str,
                 const std::string &prefix,
                 const tim::cs cs = tim::cs::sensitive);

/**
 * Проверка, заканчивается ли \a str на \a suffix.
 *
 * \param str Проверяемая строка.
 * \param suffix Искомый суффикс.
 * \param cs Чувствительность к регистру.
 * \return true, если \a str заканчивается на \a suffix.
 */
bool ends_with(const std::string &str,
               const std::string &suffix,
               const tim::cs cs = tim::cs::sensitive);

/**
 * Обрезает указанные символы с правого конца строки.
 *
 * \param s Исходная строка.
 * \param delimiters C-строка с обрезаемыми символами (по умолчанию пробельные).
 * \return Копия \a s без правых-делимитеров.
 */
std::string trim_right(const std::string &s, const char *delimiters = " \t\r\n");

/**
 * Обрезает указанные символы с левого конца строки.
 *
 * \param s Исходная строка.
 * \param delimiters Обрезаемые символы.
 * \return Копия без левых-делимитеров.
 */
std::string trim_left(const std::string &s, const char *delimiters = " \t\r\n");

/**
 * Обрезает указанные символы с обоих концов.
 *
 * \param s Исходная строка.
 * \param delimiters Обрезаемые символы.
 * \return Копия без концевых делимитеров.
 */
std::string trim(const std::string &s, const char *delimiters = " \t\r\n");

/**
 * Делает trim() и заменяет повторяющиеся пробельные на одиночные.
 *
 * \param s Исходная строка.
 * \return "Упрощённая" строка.
 */
std::string simplify(const std::string &s);

/**
 * Переносы по позиции: вставляет '\\n' каждые \a pos символов.
 *
 * \param s Строка, изменяемая по месту.
 * \param pos Позиция переноса (длина строки выводимого блока).
 */
void wrap(std::string &s, std::size_t pos);

/**
 * Обратная операция к wrap(): убирает '\\n' и склеивает в одну строку.
 *
 * \param s Исходная строка.
 * \return Строка без '\\n'.
 */
std::string unwrap(const std::string &s);

/**
 * Удаляет PEM-обрамление (BEGIN/END и переносы строк), оставляя только
 * base64-данные.
 *
 * \param pem PEM-строка.
 * \return Очищенный base64-блок.
 */
std::string strip_pem(const std::string &pem);

/**
 * printf-like форматирование в std::string (va_list-вариант).
 *
 * \param s Строка-результат.
 * \param format printf-формат.
 * \param args va_list аргументов.
 * \return Количество записанных символов.
 */
int vsprintf(std::string &s, const char *format, va_list args);

/**
 * printf-like форматирование, возвращающее std::string.
 *
 * \param format printf-формат.
 * \param args va_list аргументов.
 * \return Готовая строка.
 */
std::string vsprintf(const char *format, va_list args);

/**
 * printf-like форматирование в существующую std::string.
 *
 * \param s Строка-результат.
 * \param format printf-формат.
 * \param ... Аргументы.
 * \return Количество записанных символов.
 */
int sprintf(std::string &s, const char *format, ...)
            __attribute__ ((format(printf, 2, 3)));

/**
 * printf-like форматирование, возвращающее std::string.
 *
 * \param format printf-формат.
 * \param ... Аргументы.
 * \return Готовая строка.
 */
std::string sprintf(const char *format, ...)
                    __attribute__ ((format(printf, 1, 2)));

/**
 * Универсальный to_string() для целочисленного типа.
 *
 * \tparam T Целочисленный тип.
 * \param value Значение.
 * \return std::to_string(value).
 */
template<typename T>
inline typename std::enable_if_t<std::is_integral_v<T>, std::string> to_string(T value);

/**
 * to_string() для пользовательского типа с методом to_string()
 * (определяется через has_to_string_v).
 *
 * \tparam T Тип с to_string() → std::string.
 * \param value Значение.
 * \return Результат value.to_string().
 */
template<class T>
inline typename std::enable_if_t<tim::has_to_string_v<T>, std::string> to_string(const T &value);

/**
 * to_string() для std::string — идентичность (для шаблонных контекстов).
 *
 * \tparam T std::string.
 * \param value Значение.
 * \return \a value без изменений.
 */
template<class T>
inline typename std::enable_if_t<std::is_same_v<T, std::string>, std::string> to_string(const T &value);

/**
 * to_string() для std::filesystem::path: возвращает .string().
 *
 * \tparam T std::filesystem::path.
 * \param value Значение.
 * \return value.string().
 */
template<class T>
inline typename std::enable_if_t<std::is_same_v<T, std::filesystem::path>, std::string> to_string(const T &value);

/**
 * Шестнадцатеричное представление беззнакового целого, MSB-first.
 *
 * \tparam Unsigned Беззнаковый целочисленный тип.
 * \param value Значение.
 * \return Hex-строка из 2 * sizeof(value) символов.
 */
template<typename Unsigned>
std::string to_hex(Unsigned value);

/**
 * Парсит одиночный hex-символ.
 *
 * \param c Символ '0'..'9'/'a'..'f'/'A'..'F'.
 * \return Числовое значение 0..15, либо 0 для невалидного символа.
 */
char from_hex(char c);

/**
 * Сериализует байтовый буфер как hex-строку (без разделителей).
 *
 * \param data Байты.
 * \return Hex-строка длиной 2 * data.size().
 */
std::string to_string(const tim::byte_vector &data);

/**
 * Парсит hex-строку в байтовый буфер.
 *
 * \param s Hex-строка чётной длины.
 * \param ok Если не nullptr: true при успешном разборе.
 * \return Байтовый буфер; пустой при ошибке.
 */
tim::byte_vector from_string(const std::string &s, bool *ok = nullptr);

/**
 * Парсит JSON-строку в nlohmann::json.
 *
 * \param json Исходный JSON-текст.
 * \param j Сюда записывается результат.
 * \return true при успешном разборе.
 */
bool from_string(const std::string &json, nlohmann::json &j);

/**
 * Парсит строку как bool ("true"/"false"/"1"/"0").
 *
 * \param s Строка.
 * \param ok Если не nullptr: true при успехе.
 * \return Распарсенное значение; false при ошибке.
 */
bool to_bool(const std::string &s, bool *ok = nullptr);

/**
 * Парсит строку как int.
 *
 * \param s Строка.
 * \param ok Если не nullptr: true при успехе.
 * \return Число; 0 при ошибке.
 */
int to_int(const std::string &s, bool *ok = nullptr);

/** Парсит строку как unsigned. \param s Строка. \param ok успех/нет. \return значение или 0. */
unsigned to_uint(const std::string &s, bool *ok = nullptr);
/** Парсит строку как long long. */
long long to_long_long(const std::string &s, bool *ok = nullptr);
/** Парсит строку как unsigned long long. */
unsigned long long to_ulong_long(const std::string &s, bool *ok = nullptr);
/** Парсит строку как float. */
float to_float(const std::string &s, bool *ok = nullptr);
/** Парсит строку как double. */
double to_double(const std::string &s, bool *ok = nullptr);

/**
 * Поведение split() при встрече пустых фрагментов.
 */
enum class split_mode
{
    keep_empty_parts = 0, ///< Пустой фрагмент попадает в результат.
    skip_empty_parts = 1  ///< Пустой фрагмент пропускается.
};

/**
 * Разбивает строку по делимитерам в выбранный контейнер.
 *
 * \tparam Collection Тип результата (vector<string>, list<string> и т.п.).
 * \param s Исходная строка.
 * \param delimiters C-строка с символами-разделителями.
 *                   Пустая — режет по одному символу.
 * \param behavior Что делать с пустыми фрагментами.
 * \return Заполненный контейнер фрагментов.
 */
template<class Collection>
Collection split(const typename Collection::value_type &s,
                 const typename Collection::value_type::value_type *delimiters = " \t\r\n",
                 tim::split_mode behavior = tim::split_mode::skip_empty_parts);

/**
 * Сахар над split<Collection>() для случая Collection == std::vector<String>.
 *
 * \tparam String Тип строки.
 * \param s Исходная строка.
 * \param delimiters Символы-разделители.
 * \param behavior Что делать с пустыми фрагментами.
 * \return std::vector<String> фрагментов.
 */
template<class String>
inline std::vector<String> split_v(const String &s,
                                   const typename String::value_type *delimiters = " \t\r\n",
                                   tim::split_mode behavior = tim::split_mode::skip_empty_parts);

/**
 * Сахар над split() для случая Collection == std::list<String>.
 *
 * \tparam String Тип строки.
 * \param s Исходная строка.
 * \param delimiters Символы-разделители.
 * \param behavior Что делать с пустыми фрагментами.
 * \return std::list<String> фрагментов.
 */
template<class String>
inline std::list<String> split_l(const String &s,
                                 const typename String::value_type *delimiters = " \t\r\n",
                                 tim::split_mode behavior = tim::split_mode::skip_empty_parts);

/**
 * Преобразует __PRETTY_FUNCTION__-подобное имя в имя класса/метода.
 *
 * \param name Исходное имя (например, "void tim::foo::bar(int)").
 * \return Сокращённое имя.
 */
std::string member_name(const char *name);

/**
 * Преобразует snake_case в dotted.case (foo_bar → foo.bar).
 *
 * \param snake Имя в snake_case.
 * \return Имя через точки.
 */
std::string to_dotted(const std::string &snake);

/**
 * Преобразует dotted.case в snake_case.
 *
 * \param dotted Имя через точки.
 * \return Имя в snake_case.
 */
std::string to_snake(const std::string &dotted);

/**
 * Центрирует строку до указанной ширины пробелами по краям.
 *
 * \param str Исходная строка.
 * \param width Желаемая ширина.
 * \return Строка длины не меньше \a width.
 */
std::string centered(const std::string &str, std::size_t width);

/**
 * Усекает строку до указанной ширины, вставляя эллипсис (…) в выбранной
 * позиции.
 *
 * \param str Исходная строка.
 * \param width Максимальная ширина результата.
 * \param el Где разместить эллипсис.
 * \return Строка длиной не больше \a width.
 */
std::string elided(const std::string &str, std::size_t width, tim::elide el = tim::elide::right);

/**
 * Способ выравнивания текста при aligned().
 */
enum class text_align
{
    left    = 0, ///< Прижать к левому краю.
    right   = 1, ///< Прижать к правому краю.
    center  = 2, ///< Центрировать.
    justify = 3  ///< По ширине, добавив пробелы внутри.
};

/**
 * Выравнивает строку по указанной ширине.
 *
 * \param str Исходная строка.
 * \param al Способ выравнивания.
 * \param width Желаемая ширина.
 * \return Выровненная строка.
 */
std::string aligned(const std::string &str,
                    tim::text_align al = tim::text_align::justify,
                    std::size_t width = 80);

/**
 * Сравнение строк двух (возможно разных) типов с учётом или без учёта
 * регистра.
 *
 * \tparam S1 Тип первой строки.
 * \tparam S2 Тип второй строки.
 * \param str1 Первая строка.
 * \param str2 Вторая строка.
 * \param cs Чувствительность к регистру.
 * \return true, если строки равны (по выбранным правилам).
 */
template<class S1, class S2>
bool equal(const S1 &str1,
           const S2 &str2,
           const tim::cs cs = tim::cs::sensitive);

/**
 * Case-insensitive strcmp для C-строк.
 *
 * \param s1 Первая C-строка.
 * \param s2 Вторая C-строка.
 * \return true, если строки равны без учёта регистра.
 */
bool strcasecmp(const char *s1, const char *s2);

/**
 * Возвращает локализованное "Да"/"Нет" в зависимости от флага.
 *
 * \param f Значение.
 * \return Указатель на статический литерал TIM_TR.
 */
inline const char *yes_no(bool f);

/**
 * Возвращает локализованное "Не задано" / "N/A".
 *
 * \return Указатель на статический литерал TIM_TR.
 */
inline const char *na();

}


// Implementation

/**
 * Делегирует на std::to_string. Сделан шаблоном, чтобы выбираться
 * SFINAE для целочисленных типов.
 */
template<typename T>
typename std::enable_if_t<std::is_integral_v<T>, std::string> tim::to_string(T value)
{
    return std::to_string(value);
}

/**
 * Делегирует на value.to_string() для типов, у которых он есть.
 */
template<class T>
typename std::enable_if_t<tim::has_to_string_v<T>, std::string> tim::to_string(const T &value)
{
    return value.to_string();
}

/**
 * Идентичная перегрузка для std::string.
 */
template<class T>
typename std::enable_if_t<std::is_same_v<T, std::string>, std::string> tim::to_string(const T &value)
{
    return value;
}

/**
 * Возвращает .string() от std::filesystem::path.
 */
template<class T>
typename std::enable_if_t<std::is_same_v<T, std::filesystem::path>, std::string> tim::to_string(const T &value)
{
    return value.string();
}

/**
 * Сериализует беззнаковое целое в hex-строку MSB-first.
 *
 * \param value Беззнаковое целое.
 * \return Hex-строка длиной 2 * sizeof(value).
 */
template<typename Unsigned>
std::string tim::to_hex(Unsigned value)
{
    static_assert(std::is_unsigned_v<Unsigned>,
                  "Unsigned must be of unsigned integer type.");

    static const char *const DIGITS = "0123456789abcdef";

    value = tim::to_be(value);

    const char *p = reinterpret_cast<const char *>(&value);

    std::string s;
    s.reserve(sizeof(value) * 2);

    for (unsigned i = 0; i < sizeof(Unsigned); ++i)
    {
        unsigned int j = (p[i] >> 4) & 0xF;
        s.push_back(DIGITS[j]);
        j = p[i] & 0xF;
        s.push_back(DIGITS[j]);
    }

    return s;
}

/**
 * Реализация split: используется и для vector, и для list-перегрузок.
 *
 * Шагает по строке, накапливая фрагменты между делимитерами. При
 * keep_empty_parts расширяет результат пустыми элементами, соответствующими
 * пустым промежуткам между делимитерами.
 */
template<class Collection>
Collection tim::split(const typename Collection::value_type &s,
                      const typename Collection::value_type::value_type *delimiters,
                      tim::split_mode behavior)
{
    Collection collection;

    // Особый случай: делимитеры пусты — режем посимвольно.
    if (!delimiters
            || !*delimiters)
    {
        for (const typename Collection::value_type::value_type c: s)
            collection.emplace_back(typename Collection::value_type(&c, 1));
        return collection;
    }

    // Начало текущего поля.
    std::size_t j = s.find_first_not_of(delimiters);

    // Вся строка — одни делимитеры.
    if (j == Collection::value_type::npos)
    {
        if (behavior == tim::split_mode::keep_empty_parts)
            collection.resize(s.size());
        return collection;
    }
    if (j > 0
            && behavior == tim::split_mode::keep_empty_parts)
        collection.resize(j);

    // Начало следующей серии делимитеров.
    std::size_t i = s.find_first_of(delimiters, j);

    while (i != Collection::value_type::npos)
    {
        collection.emplace_back(s.substr(j, i - j));
        j = s.find_first_not_of(delimiters, i);

        // Хвост — одни делимитеры.
        if (j == Collection::value_type::npos)
        {
            if (behavior == tim::split_mode::keep_empty_parts)
                collection.resize(collection.size() + s.size() - i);
            return collection;
        }

        if (j > i + 1
                && behavior == tim::split_mode::keep_empty_parts)
            collection.resize(collection.size() + j - (i + 1));

        i = s.find_first_of(delimiters, j);
    }

    collection.emplace_back(s.substr(j));
    return collection;
}

/**
 * Шаблонный фасад над split<>() для случая std::vector<String>.
 */
template<class String>
std::vector<String> tim::split_v(const String &s,
                                 const typename String::value_type *delimiters,
                                 tim::split_mode behavior)
{
    return tim::split<std::vector<String>>(s, delimiters, behavior);
}

/**
 * Шаблонный фасад над split<>() для случая std::list<String>.
 */
template<class String>
std::list<String> tim::split_l(const String &s,
                               const typename String::value_type *delimiters,
                               tim::split_mode behavior)
{
    return tim::split<std::list<String>>(s, delimiters, behavior);
}

/**
 * Сравнение строк двух типов. Для cs::sensitive используется
 * оператор ==; для cs::insensitive — std::equal с tolower-предикатом.
 */
template<class S1, class S2>
bool tim::equal(const S1 &str1,
                const S2 &str2,
                const tim::cs cs)
{
    if (str1.size() != str2.size())
        return false;

    return cs == tim::cs::sensitive
                ? str1 == str2
                : std::equal(str2.cbegin(), str2.cend(),
                              str1.cbegin(),
                              [](char a, char b)
                              {
                                    return std::tolower(a) == std::tolower(b);
                              });
}

/**
 * Локализованное "Да"/"Нет".
 */
const char *tim::yes_no(bool f)
{
    return f
                ? TIM_TR("Yes"_en, "Да"_ru)
                : TIM_TR("No"_en, "Нет"_ru);
}

/**
 * Локализованное "Не задано" / "N/A".
 */
const char *tim::na()
{
    return TIM_TR("N/A"_en, "Не задано"_ru);
}
