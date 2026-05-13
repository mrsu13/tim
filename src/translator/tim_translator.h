#pragma once

#include "tim_language.h"
#include "tim_translation.h"

#include "tim_pimpl.h"


namespace tim
{

namespace p
{

struct translator;

}

/**
 * Глобальный синглтон-переводчик для TIM_TR().
 *
 * Хранит выбранный язык; translate() ищет в переданной карте
 * tim::translations нужный вариант и возвращает указатель на строку.
 * Не делает аллокаций — все строки литералы из тел вызовов TIM_TR.
 */
class translator
{

public:

    ~translator();

    static const tim::translator &instance();

    static const char *translate(const tim::translations &translations,
                                 const char *file_path, int line);

    static void set_language(tim::language lang);

    static tim::language language();

private:

    translator();

    /** PIMPL: текущий выбранный язык. */
    tim::pimpl<tim::p::translator> _d;
};

}

/**
 * Сахар над tim::translator::translate(): принимает пары операторов
 * пользовательских литералов _en/_ru и подставляет место вызова.
 *
 * \param ... Список пар (например, "Hello"_en, "Привет"_ru).
 */
#define TIM_TR(...) tim::translator::translate({ __VA_ARGS__ }, __FILE__, __LINE__)
