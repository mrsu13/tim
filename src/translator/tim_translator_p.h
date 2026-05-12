#pragma once

#include "tim_language.h"


namespace tim::p
{

/**
 * Внутреннее состояние tim::translator (PIMPL).
 */
struct translator
{
    /** Текущий выбранный язык. По умолчанию русский. */
    tim::language _language = tim::language::ru_ru;
};

}
