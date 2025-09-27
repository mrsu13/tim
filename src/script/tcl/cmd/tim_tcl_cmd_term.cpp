#include "tim_tcl_cmd_term.h"

#include "tim_a_terminal.h"
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

    tcl->terminal()->clear();

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

    for (std::size_t c = 0; c <= tcl->terminal()->color_count(); ++c)
    {
        if (c > 0
                && c % ITEMS_PER_LINE == 0)
            tcl->terminal()->write("\n", 1);

        tcl->terminal()->printf("%3zu", c);
        tcl->terminal()->cprintf(tim::color{}, tcl->terminal()->color(c), "%s", "  ");
        tcl->terminal()->write(" ", 1);
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
