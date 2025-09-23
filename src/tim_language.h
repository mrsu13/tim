#pragma once

#include "tim_enum.h"
#include "tim_flags.h"


namespace tim
{

/**
 * Languages supported by tim::translator.
 *
 * \sa tim::translator TIM_TR()
 */
enum class language
{
    Unknown = 0, ///< The language is not defoned.

    en_US = 1 << 0, ///< USA English.
    ru_RU = 1 << 1  ///< Russian.
};

using languages = tim::flags<tim::language>;

}

TIM_DECLARE_META_ENUM(tim::language,
    TIM_ENUM_ITEM(tim::language::en_US, TIM_TR("USA English"_en, "Английский США"_ru)),
    TIM_ENUM_ITEM(tim::language::ru_RU, TIM_TR("Russian"_en, "Русский"_ru))
);
