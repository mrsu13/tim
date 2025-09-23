#include "tim_translator.h"

#include "tim_translator_p.h"

#include "tim_trace.h"
#include "tim_translation.h"

#include <cassert>


/**
 * \class tim::translator
 * \brief Support text strings translations the source code contains.
 *
 * You shoule never use this class directly. Instead use TIM_TR() macro.
 *
 * \sa tim::translation
 */

// Public

const tim::translator &tim::translator::instance()
{
    static tim::translator t{};
    return t;
}

/**
 * \param translations Translations.
 * \param file_path Path to the source code file where this method call is located.
 * \param line Line number where this method call is located.
 * \return Translation on the base of the current locale.
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
    TIM_TRACE(Warning,
              "No translation for language %d defined in file '%s' at line %d.",
              static_cast<int>(tim::translator::instance()._d->_language),
              file_path,
              line);
#endif

    return translations.cbegin()->second;
}


// Private

tim::translator::translator()
    : _d(new tim::p::translator())
{
}

tim::translator::~translator() = default;
