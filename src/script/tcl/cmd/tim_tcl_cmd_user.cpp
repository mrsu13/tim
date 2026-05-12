#include "tim_tcl_cmd_user.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
#include "tim_tcl_cmd.h"
#include "tim_translator.h"
#include "tim_tcl.h"
#include "tim_uuid.h"

#include "lil.hpp"

#include <cassert>


// Static

static lil_value_t tim_tcl_cmd_setnick(lil_t lil,
                                       std::size_t argc,
                                       lil_value_t *argv)
{
    (void) argv;

    if (argc != 1)
    {
        lil_set_error(lil,
                      TIM_TR("Expected nick"_en,
                             "Ожидаем параметр nick"_ru));
        return nullptr;
    }

    const std::string nick = lil_to_string(argv[0]);
    const tim::tcl *tcl = (const tim::tcl *)lil_get_data(lil);
    assert(tcl);

    tcl->mqtt().publish(tim::mqtt_topic("user/setnick") / tcl->user_id().to_string(tim::uuid::format::NoBrackets),
                        nick.c_str(), nick.size());

    return nullptr;
}

static lil_value_t tim_tcl_cmd_seticon(lil_t lil,
                                       std::size_t argc,
                                       lil_value_t *argv)
{
    (void) argv;

    if (argc != 1)
    {
        lil_set_error(lil,
                      TIM_TR("Expected icon"_en,
                             "Ожидаем параметр icon"_ru));
        return nullptr;
    }

    const std::string icon = lil_to_string(argv[0]);
    const tim::tcl *tcl = (const tim::tcl *)lil_get_data(lil);
    assert(tcl);

    tcl->mqtt().publish(tim::mqtt_topic("user/seticon") / tcl->user_id().to_string(tim::uuid::format::NoBrackets),
                        icon.c_str(), icon.size());

    return nullptr;
}

static lil_value_t tim_tcl_cmd_subscribe(lil_t lil,
                                         std::size_t argc,
                                         lil_value_t *argv)
{
    if (argc != 1)
    {
        lil_set_error(lil,
                      TIM_TR("Expected publisher UUID"_en,
                             "Ожидаем UUID издателя"_ru));
        return nullptr;
    }

    const std::string arg = lil_to_string(argv[0]);
    const tim::uuid publisher(arg);
    if (!publisher.valid())
    {
        lil_set_error(lil,
                      TIM_TR("Invalid UUID"_en,
                             "Некорректный UUID"_ru));
        return nullptr;
    }

    const tim::tcl *tcl = (const tim::tcl *)lil_get_data(lil);
    assert(tcl);

    tcl->mqtt().publish(tim::mqtt_topic("user/subscribe") / tcl->user_id().to_string(tim::uuid::format::NoBrackets),
                        publisher.to_string(tim::uuid::format::NoBrackets));

    return nullptr;
}

static lil_value_t tim_tcl_cmd_unsubscribe(lil_t lil,
                                           std::size_t argc,
                                           lil_value_t *argv)
{
    if (argc != 1)
    {
        lil_set_error(lil,
                      TIM_TR("Expected publisher UUID"_en,
                             "Ожидаем UUID издателя"_ru));
        return nullptr;
    }

    const std::string arg = lil_to_string(argv[0]);
    const tim::uuid publisher(arg);
    if (!publisher.valid())
    {
        lil_set_error(lil,
                      TIM_TR("Invalid UUID"_en,
                             "Некорректный UUID"_ru));
        return nullptr;
    }

    const tim::tcl *tcl = (const tim::tcl *)lil_get_data(lil);
    assert(tcl);

    tcl->mqtt().publish(tim::mqtt_topic("user/unsubscribe") / tcl->user_id().to_string(tim::uuid::format::NoBrackets),
                        publisher.to_string(tim::uuid::format::NoBrackets));

    return nullptr;
}


// Public

void tim::tcl_add_user(lil_t lil)
{
    assert(lil);

    TIM_TCL_REGISTER(lil, setnick);
    TIM_TCL_REGISTER(lil, seticon);
    TIM_TCL_REGISTER(lil, subscribe);
    TIM_TCL_REGISTER(lil, unsubscribe);
}
