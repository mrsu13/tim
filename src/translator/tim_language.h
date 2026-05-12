#pragma once

// Не tim_json.h: тот включает tim_translator.h → tim_language.h, что
// замыкает цикл. nlohmann/json.hpp — самодостаточный third-party-заголовок.
#include "nlohmann/json.hpp"

#include <string>


namespace tim
{

/**
 * Languages supported by tim::tr.
 *
 * \see tim::tr TIM_TR()
 */
enum class language
{
    Unknown = 0, ///< Not defined.

    en_US = 1 << 0, ///< English USA.
    ru_RU = 1 << 1  ///< Russian.
};

// JSON-сериализация: в файле конфигурации язык хранится как короткая
// строка "en"/"ru" — оператору так понятнее, чем числовой код.
inline void to_json(nlohmann::json &j, const tim::language &lang)
{
    switch (lang)
    {
        case tim::language::en_US: j = "en"; return;
        case tim::language::ru_RU: j = "ru"; return;
        case tim::language::Unknown: break;
    }
    j = "ru";
}

inline void from_json(const nlohmann::json &j, tim::language &lang)
{
    const std::string s = j.get<std::string>();
    if (s == "en" || s == "en_US")
        lang = tim::language::en_US;
    else
        lang = tim::language::ru_RU;
}

}
