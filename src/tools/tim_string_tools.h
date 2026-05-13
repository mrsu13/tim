#pragma once

#include "tim_elide.h"
#include "tim_translator.h"

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
 * Заменяет все вхождения \a old_str на \a new_str в \a str (in-place).
 *
 * \param str Строка, изменяемая по месту.
 * \param old_str Подстрока для поиска.
 * \param new_str Подстрока-замена.
 */
void replace(std::string &str,
             const std::string &old_str,
             const std::string &new_str);

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
 * Парсит строку как int.
 *
 * \param s Строка.
 * \param ok Если не nullptr: true при успехе.
 * \return Число; 0 при ошибке.
 */
int to_int(const std::string &s, bool *ok = nullptr);

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
 * Возвращает локализованное "Не задано" / "N/A".
 *
 * \return Указатель на статический литерал TIM_TR.
 */
inline const char *na();

}


// Implementation

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
 * Локализованное "Не задано" / "N/A".
 */
const char *tim::na()
{
    return TIM_TR("N/A"_en, "Не задано"_ru);
}
