#pragma once

#include "tim_language.h"
#include "tim_translation.h"

#include <memory>


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

    /** Деструктор. */
    ~translator();

    /**
     * Возвращает живой singleton-переводчик.
     *
     * \return Ссылка на единственный экземпляр.
     */
    static const tim::translator &instance();

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
    static const char *translate(const tim::translations &translations,
                                 const char *file_path, int line);

    /**
     * Глобальный переключатель языка для TIM_TR. Вызывать на старте, до
     * любых пользовательских TIM_TR-вызовов (например, после загрузки
     * tim::settings и до создания подсистем).
     *
     * \param lang Новый язык.
     */
    static void set_language(tim::language lang);

    /** \return Текущий выбранный язык переводчика. */
    static tim::language language();

private:

    /** Закрытый конструктор: только через instance(). */
    translator();

    /** PIMPL: текущий выбранный язык. */
    std::unique_ptr<tim::p::translator> _d;
};

}

/**
 * Сахар над tim::translator::translate(): принимает пары операторов
 * пользовательских литералов _en/_ru и подставляет место вызова.
 *
 * \param ... Список пар (например, "Hello"_en, "Привет"_ru).
 */
#define TIM_TR(...) tim::translator::translate({ __VA_ARGS__ }, __FILE__, __LINE__)
