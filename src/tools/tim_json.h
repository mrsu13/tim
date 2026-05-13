#pragma once

#include "tim_trace.h"
#include "tim_translator.h"


/**
 * \name Макро-перехватчики nlohmann/json
 *
 * TIM собирается с -fno-exceptions; nlohmann/json по умолчанию использует
 * исключения. Через эти макросы переопределяем "try/catch/throw" так,
 * чтобы любое исключение приводило к печати в лог и std::abort().
 * При -DJSON_NOEXCEPTION (см. Makefile) ветка throw не должна срабатывать
 * на штатном пути; это страховка.
 * \{
 */
#define JSON_TRY_USER if(true)
#define JSON_CATCH_USER(exception) if(false)
#define JSON_THROW_USER(exception) \
    { \
        TIM_TRACE(error, TIM_TR("JSON error: %s"_en, "Ошибка JSON: %s"_ru), (exception).what()); \
        std::abort(); \
    }
/** \} */

#include "nlohmann/json.hpp"
