#pragma once

typedef struct _lil_t *lil_t;

namespace tim
{

/**
 * Регистрирует общие Tcl-команды (универсальные, не связанные с конкретным
 * сервисом): /quit, /clear и т.п.
 *
 * \param lil LIL-сессия.
 */
void tcl_add_general(lil_t lil);

}
