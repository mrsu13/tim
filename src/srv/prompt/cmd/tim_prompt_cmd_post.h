#pragma once

typedef struct _lil_t *lil_t;

namespace tim::prompt
{

/**
 * Регистрирует команды, связанные с публикациями: /react.
 *
 * Команды достают prompt_service через tcl->user_data().
 *
 * \param lil LIL-сессия.
 */
void register_post_cmds(lil_t lil);

}
