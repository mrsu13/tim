#include "tim_tcl_cmd_term.h"

#include "tim_a_telnet_service.h"
#include "tim_tcl.h"
#include "tim_tcl_cmd.h"
#include "tim_translator.h"

#include "lil.hpp"

#include <cassert>


// Static

static lil_value_t tim_tcl_cmd_clear(lil_t lil, size_t argc, lil_value_t *argv)
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

    tcl->telnet()->clear();

    return nullptr;
}

static lil_value_t tim_tcl_cmd_palette256(lil_t lil, size_t argc, lil_value_t *argv)
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

    static const std::size_t ITEMS_PER_LINE = 10;

    for (std::size_t c = 0; c <= 0xFF; ++c)
    {
        if (c > 0
                && c % ITEMS_PER_LINE == 0)
            tcl->telnet()->write("\n", 1);

        tcl->telnet()->printf("%3zu", c);
        tcl->telnet()->cprintf(tim::color{}, tim::a_telnet_service::color(c), "%s", "  ");
        tcl->telnet()->write(" ", 1);
    }

    return nullptr;
}


// Public

void tim::tcl_add_term(lil_t lil)
{
    assert(lil);

    TIM_TCL_REGISTER(lil, clear);
    TIM_TCL_REGISTER(lil, palette256);
}
