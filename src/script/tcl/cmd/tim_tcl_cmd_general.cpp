#include "tim_tcl_cmd_general.h"

#include "tim_tcl.h"
#include "tim_tcl_cmd.h"
#include "tim_translator.h"

#include "lil.hpp"

#include <cassert>


// Static

/**
 * Команда `/quit`: закрывает ТЕКУЩУЮ сессию, не трогая остальной
 * сервер. Конкретное действие выполняет prompt_service, регистрирующий
 * обработчик через tcl->set_quit_handler.
 *
 * \param lil LIL-сессия.
 * \param argc Количество аргументов; команда не принимает ни одного.
 * \param argv Аргументы (не используются).
 * \return nullptr (команда не возвращает значения).
 */
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

/**
 * Регистрирует общие Tcl-команды в указанной LIL-сессии. Сейчас
 * это только `quit`; другие общие команды добавлять сюда.
 */
void tim::tcl_add_general(lil_t lil)
{
    assert(lil);

    TIM_TCL_REGISTER(lil, quit);
}
