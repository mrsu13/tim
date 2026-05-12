#pragma once

#include <cstdint>
#include <unordered_map>


namespace tim
{

/**
 * Одна пара (язык, строка). Используется операторами пользовательских
 * литералов _en и _ru, чтобы передавать перевод в tim::translator.
 */
using translation = std::pair<std::int64_t, const char *>;

/**
 * Полная карта переводов одной строки. Ключ — int64-код языка
 * (см. tim::language); значение — литерал.
 */
using translations = std::unordered_map<std::int64_t, const char *>;

}

/**
 * Декларация литерала _en. Возвращает tim::translation для английского
 * варианта строки. \a n игнорируется (длина выводится из самой строки).
 *
 * \param s C-строка (литерал в коде).
 * \param n Длина литерала; не используется.
 * \return Пара (lang_id=1, указатель s).
 */
inline constexpr tim::translation operator ""_en(const char *s, std::size_t n)
#ifdef __GNUC__
    __attribute__((nonnull(1)))
#endif
;

/**
 * Декларация литерала _ru. Возвращает tim::translation для русского
 * варианта строки. \a n игнорируется.
 *
 * \param s C-строка (литерал в коде).
 * \param n Длина литерала; не используется.
 * \return Пара (lang_id=2, указатель s).
 */
inline constexpr tim::translation operator ""_ru(const char *s, std::size_t n)
#ifdef __GNUC__
    __attribute__((nonnull(1)))
#endif
;


/**
 * Определение литерала _en. Lang_id жёстко 1 — соответствует
 * tim::language::en_us; константу из enum нельзя использовать в этом
 * заголовке из-за циклических включений.
 */
inline constexpr tim::translation operator ""_en(const char *s, std::size_t n)
{
    (void) n;
    return tim::translation{ 1, s };
}

/**
 * Определение литерала _ru. Lang_id жёстко 2 — соответствует
 * tim::language::ru_ru.
 */
inline constexpr tim::translation operator ""_ru(const char *s, std::size_t n)
{
    (void) n;
    return tim::translation{ 2, s };
}
