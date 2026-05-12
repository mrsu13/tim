#pragma once

#include <cassert>


/**
 * Сахар для регистрации Tcl-команды в LIL-сессии. При неудаче
 * lil_register возвращает <= 0 — попадание в эту ветку означает баг
 * (имя занято / пустая лямбда), такие случаи мы хотим увидеть в
 * debug-сборке через assert и не давать запуститься в release.
 *
 * \param lil LIL-сессия (lil_t).
 * \param name Идентификатор команды; нужна функция tim_tcl_cmd_<name>.
 */
#define TIM_TCL_REGISTER(lil, name) \
if (lil_register((lil), #name, tim_tcl_cmd_##name) <= 0) \
    return assert(0 && "Failed to register TCL command '" #name "'.");
