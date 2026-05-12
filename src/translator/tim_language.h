#pragma once

// Не tim_json.h: тот включает tim_translator.h → tim_language.h, что
// замыкает цикл. nlohmann/json.hpp — самодостаточный third-party-заголовок.
#include "nlohmann/json.hpp"

#include <string>


namespace tim
{

/**
 * Языки, на которых TIM выводит сообщения (TIM_TR/tim::translator).
 *
 * \see TIM_TR
 */
enum class language
{
    unknown = 0,    ///< Не определён.
    en_us = 1 << 0, ///< Английский (США).
    ru_ru = 1 << 1  ///< Русский.
};

/**
 * nlohmann-сериализация tim::language → JSON.
 *
 * В файле конфигурации язык хранится как короткая строка "en"/"ru" —
 * оператору так понятнее, чем числовой код.
 *
 * \param j Цель сериализации.
 * \param lang Значение для записи.
 */
inline void to_json(nlohmann::json &j, const tim::language &lang)
{
    switch (lang)
    {
        case tim::language::en_us: j = "en"; return;
        case tim::language::ru_ru: j = "ru"; return;
        case tim::language::unknown: break;
    }
    j = "ru";
}

/**
 * Десериализация JSON → tim::language.
 *
 * \param j Источник: строка "en"/"en_US" → en_us, всё прочее → ru_ru
 *          (безопасный fallback на дефолтный язык).
 * \param lang Сюда записывается результат.
 */
inline void from_json(const nlohmann::json &j, tim::language &lang)
{
    const std::string s = j.get<std::string>();
    if (s == "en" || s == "en_US")
        lang = tim::language::en_us;
    else
        lang = tim::language::ru_ru;
}

}
