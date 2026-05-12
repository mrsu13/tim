#include "tim_tcl_cmd_post.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
#include "tim_prompt_service.h"
#include "tim_string_tools.h"
#include "tim_tcl.h"
#include "tim_tcl_cmd.h"
#include "tim_translator.h"
#include "tim_uuid.h"

#include "lil.hpp"

#include <cassert>
#include <string>


// Static

// /react [weight]
// Реакция на последнее увиденное в чате сообщение. weight по умолчанию 1;
// weight = 0 снимает ранее выставленную реакцию.
static lil_value_t tim_tcl_cmd_react(lil_t lil,
                                     std::size_t argc,
                                     lil_value_t *argv)
{
    int weight = 1;
    if (argc > 1)
    {
        lil_set_error(lil,
                      TIM_TR("Expected optional weight"_en,
                             "Ожидаем необязательный параметр weight"_ru));
        return nullptr;
    }
    if (argc == 1)
    {
        bool ok = false;
        weight = tim::to_int(lil_to_string(argv[0]), &ok);
        if (!ok)
        {
            lil_set_error(lil,
                          TIM_TR("Weight must be an integer"_en,
                                 "weight должен быть целым числом"_ru));
            return nullptr;
        }
    }

    tim::tcl *tcl = (tim::tcl *)lil_get_data(lil);
    assert(tcl);

    // /react цепляется к чат-сессии: владелец tim::tcl (prompt_service)
    // регистрирует себя как user_data.
    tim::prompt_service *prompt = (tim::prompt_service *)tcl->user_data();
    if (!prompt)
    {
        lil_set_error(lil,
                      TIM_TR("Command requires a chat session context."_en,
                             "Команда должна выполняться в контексте чат-сессии."_ru));
        return nullptr;
    }
    if (!prompt->last_seen_post().valid())
    {
        lil_set_error(lil,
                      TIM_TR("No post to react to yet"_en,
                             "Нет сообщений, на которые можно отреагировать"_ru));
        return nullptr;
    }

    const tim::mqtt_topic topic =
        tim::mqtt_topic("react")
            / prompt->last_seen_post().to_string(tim::uuid::format::NoBrackets)
            / prompt->user_id().to_string(tim::uuid::format::NoBrackets);
    prompt->mqtt().publish(topic, std::to_string(weight));

    return nullptr;
}


// Public

void tim::tcl_add_post(lil_t lil)
{
    assert(lil);

    TIM_TCL_REGISTER(lil, react);
}
