#pragma once

typedef struct _lil_t *lil_t;

namespace tim
{

/**
 * Регистрирует Tcl-команды для управления терминалом: /puts, /palette256
 * и подобные.
 *
 * \param lil LIL-сессия.
 */
void tcl_add_term(lil_t lil);

}
