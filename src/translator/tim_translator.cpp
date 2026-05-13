#include "tim_translator.h"

#include "tim_translator_p.h"

#include "tim_trace.h"
#include "tim_translation.h"

#include <cassert>


/**
 * \class tim::translator
 * \brief Поддержка переводов текстовых строк, содержащихся в исходном коде.
 *
 * Никогда не используйте этот класс напрямую. Вместо этого используйте
 * макрос TIM_TR().
 *
 * \sa tim::translation
 */

// Public

/**
 * Возвращает живой singleton-переводчик.
 *
 * \return Ссылка на единственный экземпляр.
 */
const tim::translator &tim::translator::instance()
{
    static tim::translator t{};
    return t;
}

/**
 * Глобальный переключатель языка для TIM_TR. Вызывать на старте, до
 * любых пользовательских TIM_TR-вызовов (например, после загрузки
 * tim::settings и до создания подсистем).
 *
 * \param lang Новый язык.
 */
void tim::translator::set_language(tim::language lang)
{
    // Через const_cast — instance() возвращает const& из соображений API;
    // язык же — единственное мутируемое состояние, выставляется один раз
    // на старте и далее остаётся read-only по факту.
    const_cast<tim::translator &>(tim::translator::instance())._d->_language = lang;
}

/** \return Текущий выбранный язык переводчика. */
tim::language tim::translator::language()
{
    return tim::translator::instance()._d->_language;
}

/**
 * Ищет перевод в карте по текущему выбранному языку. При отсутствии
 * перевода для текущего языка возвращается первый из карты, а в
 * debug-сборке логируется warning о пропущенной локализации.
 *
 * \param translations Карта lang_id → строка. Не должна быть пустой.
 * \param file_path Путь к файлу с вызовом (для debug-сообщения).
 * \param line Номер строки с вызовом.
 * \return Указатель на C-строку перевода (литерал, не освобождать).
 */
const char *tim::translator::translate(const tim::translations &translations,
                                       const char *file_path, int line)
{
    assert(!translations.empty());
    assert(file_path);
    assert(line > 0);

    tim::translations::const_iterator f
        = translations.find((std::int64_t)tim::translator::instance()._d->_language);
    if (f != translations.cend())
        return f->second;

#ifdef TIM_DEBUG
    TIM_TRACE(warning,
              "No translation for language %d defined in file '%s' at line %d.",
              static_cast<int>(tim::translator::instance()._d->_language),
              file_path,
              line);
#endif

    return translations.cbegin()->second;
}


// Private

/** Закрытый конструктор: только через instance(). */
tim::translator::translator()
    : _d(new tim::p::translator())
{
}

/** Деструктор. */
tim::translator::~translator() = default;
