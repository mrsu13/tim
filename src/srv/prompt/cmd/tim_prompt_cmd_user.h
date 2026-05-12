#pragma once

typedef struct _lil_t *lil_t;

namespace tim::prompt
{

// Регистрирует "пользовательские" команды чата в указанной LIL-сессии:
// /setnick, /seticon, /subscribe, /unsubscribe, /subscriptions,
// /subscribers. Команды достают prompt_service через tcl->user_data().
void register_user_cmds(lil_t lil);

}
