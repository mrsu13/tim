#include "tim_tcl_cmd_general.h"

#include "tim_application.h"
#include "tim_tcl_cmd.h"
#include "tim_translator.h"

#include "lil.hpp"

#include <cassert>


// Static

#ifdef TIM_DEBUG

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

    tim::app()->quit();

    return nullptr;
}

#endif


// Public

void tim::tcl_add_general(lil_t lil)
{
    assert(lil);

#ifdef TIM_DEBUG
    TIM_TCL_REGISTER(lil, quit);
#endif
}
