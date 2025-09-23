#pragma once

#include "tim_trace.h"
#include "tim_translator.h"


#define JSON_TRY_USER if(true)
#define JSON_CATCH_USER(exception) if(false)
#define JSON_THROW_USER(exception) \
    { \
        TIM_TRACE(Error, TIM_TR("JSON error: %s"_en, "Ошибка JSON: %s"_ru), (exception).what()); \
        std::abort(); \
    }

#include "nlohmann/json.hpp"
