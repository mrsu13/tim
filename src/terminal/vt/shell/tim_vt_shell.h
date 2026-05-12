#pragma once

#include <memory>
#include <string>


namespace tim
{

class a_script_engine;
class vt;

namespace p
{

struct vt_shell;

}

/**
 * Минимальный интерактивный шелл поверх VT-терминала.
 *
 * Печатает приглашение, читает строку через line_edit, и при Enter
 * передаёт её в a_script_engine на выполнение. Наследники
 * (prompt_shell) могут перехватывать ввод не-команд через
 * accept_command().
 */
class vt_shell
{

public:

    /**
     * Конструктор. Создаёт line_edit, загружает историю с диска,
     * рисует welcome-баннер.
     *
     * \param term Терминал, в который шелл отрисовывает.
     * \param engine Скрипт-движок, исполняющий введённые команды.
     */
    vt_shell(tim::vt *term, tim::a_script_engine *engine);

    /** Виртуальный деструктор: сохраняет историю на диск. */
    virtual ~vt_shell();

    /** \return Терминал, к которому привязан шелл. */
    tim::vt *terminal() const;

    /**
     * Печатает '\\n' и заново выводит приглашение. Используется для
     * "вытолкнутых" сверху сообщений (плашки сообщений, уведомления).
     */
    void new_line();

    /**
     * Передаёт сырые байты ввода в line_edit. Выполняет команду при
     * нажатии Enter; иначе обновляет приглашение.
     *
     * \param data Сырые байты ввода.
     * \param size Размер.
     * \return true, если шелл хочет продолжать работу; false — Ctrl+D
     *         или ошибка.
     */
    bool write(const char *data, std::size_t size);

    /**
     * Скрыть приглашение и введённую строку (если есть), чтобы напечатать
     * в терминал что-то "поверх". show_input() возвращает приглашение.
     */
    void hide_input();
    /** Восстанавливает приглашение после hide_input(). */
    void show_input();

protected:

    /**
     * Решает, считать ли введённую строку командой для engine или
     * чем-то ещё (наследник может перехватить и испустить свой сигнал).
     *
     * \param line Введённая строка (без \\n).
     * \param command Сюда записывается текст команды для engine при
     *                возврате true.
     * \return true — выполнить как команду; false — наследник обработал
     *         иначе (например, испустил "сообщение").
     */
    virtual bool accept_command(const std::string &line, std::string &command);

private:

    /** PIMPL: line_edit, путь к истории, ссылка на engine. */
    std::unique_ptr<tim::p::vt_shell> _d;
};

}
