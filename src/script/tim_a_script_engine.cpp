#include "tim_a_script_engine.h"

#include "tim_a_script_engine_p.h"

#include <cassert>


// Public

/**
 * Конструктор. Сохраняет имя языка и указатель на терминал.
 *
 * Базовый класс абстрактный за счёт чисто виртуальных методов, поэтому
 * конструктор реально вызывается только из наследников.
 *
 * \param language Имя языка (например, "Tcl") — для логов и UI.
 * \param term Терминал, в который команды могут писать.
 */
tim::a_script_engine::a_script_engine(const std::string &language,
                                      tim::a_terminal *term)
    : _d(new tim::p::a_script_engine())
{
    assert(!language.empty() && "Script language name must not be empty.");
    assert(term);

    _d->_language = language;
    _d->_terminal = term;
}

/** Виртуальный деструктор. PIMPL разрушает _d автоматически. */
tim::a_script_engine::~a_script_engine() = default;

/** \return Имя языка скрипт-движка. */
const std::string &tim::a_script_engine::language() const
{
    return _d->_language;
}

/** \return Терминал, к которому привязан движок. */
tim::a_terminal *tim::a_script_engine::terminal() const
{
    return _d->_terminal;
}

/**
 * Возвращает список автодополнений для префикса. Реализация по
 * умолчанию — пустой список; наследники (tim::tcl) могут
 * переопределить.
 *
 * \param prefix Префикс для поиска.
 * \return Список имён, начинающихся с prefix; пустой по умолчанию.
 */
std::vector<std::string> tim::a_script_engine::complete(const std::string &prefix) const
{
    (void) prefix;

    return {};
}
