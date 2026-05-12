#pragma once

#include "tim_color.h"
#include "tim_signal.h"
#include "tim_vt_shell.h"


namespace tim
{

namespace p
{

struct prompt_shell;

}

/**
 * Шелл чата: расширяет vt_shell для отрисовки "плашек" сообщений и
 * перехвата ввода обычных строк как постов.
 *
 * Испускает сигнал posted с введённым текстом, если пользователь
 * нажал Enter в режиме обычного ввода (не команды /...).
 */
class prompt_shell : public tim::vt_shell
{

public:

    /**
     * Сигнал: пользователь набрал и отправил текст (не команду).
     * Аргумент — введённый текст без завершающего \\n.
     */
    tim::signal<const std::string & /* text */> posted;

    /**
     * Конструктор.
     *
     * \param term VT-терминал, в который шелл отрисовывает.
     * \param engine Скрипт-движок (Tcl) для обработки /-команд.
     */
    prompt_shell(tim::vt *term, tim::a_script_engine *engine);

    /** Деструктор. */
    ~prompt_shell();

    /**
     * Рисует "плашку" сообщения: заголовок (ник автора) + тело.
     *
     * Если задан \a marker_color, перед заголовком в полосе плашки
     * рисуется маркер ('★') этим цветом; сам заголовок отображается
     * обычным авто-контрастным цветом текста. Используется для пометки
     * сообщений от пользователей, на которых вы подписаны.
     *
     * \param title Заголовок плашки (ник/Я).
     * \param text Тело сообщения.
     * \param bg_color Цвет фона плашки (transparent — без фона).
     * \param marker_color Цвет маркера-звёздочки (пусто — нет маркера).
     */
    void cloud(const std::string &title,
               const std::string &text,
               const tim::color &bg_color = tim::color::transparent(),
               const tim::color &marker_color = tim::color{});

protected:

    /**
     * Принимает введённую строку как команду или как обычный текст.
     *
     * Если строка начинается с COMMAND_PREFIX, возвращает true
     * и кладёт текст команды в \a command для исполнения Tcl-движком.
     * Иначе испускает сигнал posted и возвращает false.
     *
     * \param line Введённая строка.
     * \param command Сюда записывается текст команды (без префикса).
     * \return true, если это команда — vt_shell исполнит её через engine.
     */
    bool accept_command(const std::string &line, std::string &command) override;

private:

    /** PIMPL: cigналы и состояние шелла. */
    std::unique_ptr<tim::p::prompt_shell> _d;
};

}
