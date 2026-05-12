#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>


namespace tim
{

class a_terminal;

namespace p
{

struct a_script_engine;

}

/**
 * Абстрактный скрипт-движок, исполняющий введённые в шелле программы.
 *
 * Конкретная реализация — tim::tcl (поверх LIL). Интерфейс описывает
 * минимум для интеграции с vt_shell: eval, прерывание, диагностику
 * ошибок, автодополнение.
 */
class a_script_engine
{

public:

    /**
     * Конструктор только для наследников через protected-видимость
     * (тут public — но базовый класс абстрактный за счёт виртуальных
     * методов).
     *
     * \param language Имя языка (например, "Tcl") — для логов и UI.
     * \param term Терминал, в который команды могут писать.
     */
    explicit a_script_engine(const std::string &language,
                             tim::a_terminal *term);

    /** Виртуальный деструктор. */
    virtual ~a_script_engine();

    /** \return Имя языка скрипт-движка. */
    const std::string &language() const;

    /** \return Терминал, к которому привязан движок. */
    tim::a_terminal *terminal() const;

    /** \return true, если сейчас выполняется eval(). */
    virtual bool evaluating() const = 0;

    /**
     * Выполняет переданный скрипт.
     *
     * \param program Исходный текст программы.
     * \param res Если не nullptr: туда записывается результат
     *            (текстовое представление последнего выражения).
     * \return true, если ошибок не возникло.
     */
    virtual bool eval(const std::string &program,
                      std::string *res = nullptr) = 0;

    /** Прерывает выполнение текущего eval. */
    virtual void break_eval() = 0;

    /** \return Префикс приглашения шелла для этого движка. */
    virtual const std::string &prompt() const = 0;

    /** \return Текст последней ошибки в выбранной локали. */
    virtual const std::string &error_msg() const = 0;

    /** \return Позиция ошибки в исходнике. */
    virtual std::size_t error_pos() const = 0;

    /**
     * Возвращает список автодополнений для префикса.
     *
     * \param prefix Префикс для поиска.
     * \return Список имён, начинающихся с prefix; пустой по умолчанию.
     */
    virtual std::vector<std::string> complete(const std::string &prefix) const;

    /** \return Множество зарезервированных ключевых слов языка. */
    virtual std::unordered_set<std::string> keywords() const = 0;

    /** \return Множество имён зарегистрированных функций (для autocomplete). */
    virtual std::unordered_set<std::string> functions() const = 0;

private:

    /** PIMPL: имя языка и указатель на терминал. */
    std::unique_ptr<tim::p::a_script_engine> _d;
};

}
