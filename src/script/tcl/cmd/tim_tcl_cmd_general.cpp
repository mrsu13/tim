#include "tim_tcl_cmd_general.h"

#include "tim_tcl.h"
#include "tim_tcl_cmd.h"
#include "tim_translator.h"

#include "lil.hpp"

#include <cassert>


// Static

// /quit
// Закрывает ТЕКУЩУЮ сессию, не трогая остальной сервер. Конкретное
// действие выполняет prompt_service, регистрирующий обработчик через
// tcl->set_quit_handler.
static lil_value_t tim_tcl_cmd_quit(lil_t lil, size_t argc, lil_value_t *argv)
{
    (void) argv;

    if (argc)
    {
        lil_set_error(lil,
                      TIM_TR("No arguments expected."_en,
                             "Команда не имеет параметров."_ru));
        return nullptr;
    }

    tim::tcl *tcl = (tim::tcl *)lil_get_data(lil);
    assert(tcl);

    tcl->request_quit();

    return nullptr;
}


// Public

void tim::tcl_add_general(lil_t lil)
{
    assert(lil);

    TIM_TCL_REGISTER(lil, quit);
}
