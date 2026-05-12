#include "tim_prompt_cmd_user.h"

#include "tim_a_terminal.h"
#include "tim_mqtt_client.h"
#include "tim_mqtt_topic.h"
#include "tim_mqtt_topics.h"
#include "tim_prompt_service.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_tcl.h"
#include "tim_tcl_cmd.h"
#include "tim_translator.h"
#include "tim_user.h"
#include "tim_uuid.h"

#include "lil.hpp"

#include <cassert>


// Static

// Достаём контекст сессии (prompt_service) из tcl->user_data().
// Если контекст не выставлен — команда не имеет смысла, возвращаем
// сообщение об ошибке.
static tim::prompt_service *session_or_error(lil_t lil, tim::tcl *&tcl_out)
{
    tcl_out = (tim::tcl *)lil_get_data(lil);
    assert(tcl_out);
    tim::prompt_service *prompt = (tim::prompt_service *)tcl_out->user_data();
    if (!prompt)
    {
        lil_set_error(lil,
                      TIM_TR("Command requires a chat session context."_en,
                             "Команда должна выполняться в контексте чат-сессии."_ru));
        return nullptr;
    }
    return prompt;
}


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
    tim::tcl *tcl = nullptr;
    tim::prompt_service *prompt = session_or_error(lil, tcl);
    if (!prompt)
        return nullptr;

    prompt->mqtt().publish(tim::topics::user_setnick(prompt->user_id()),
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
    tim::tcl *tcl = nullptr;
    tim::prompt_service *prompt = session_or_error(lil, tcl);
    if (!prompt)
        return nullptr;

    prompt->mqtt().publish(tim::topics::user_seticon(prompt->user_id()),
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

    tim::tcl *tcl = nullptr;
    tim::prompt_service *prompt = session_or_error(lil, tcl);
    if (!prompt)
        return nullptr;

    prompt->mqtt().publish(tim::topics::user_subscribe(prompt->user_id()),
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

    tim::tcl *tcl = nullptr;
    tim::prompt_service *prompt = session_or_error(lil, tcl);
    if (!prompt)
        return nullptr;

    prompt->mqtt().publish(tim::topics::user_unsubscribe(prompt->user_id()),
                           publisher.to_string(tim::uuid::format::NoBrackets));

    return nullptr;
}


// /subscriptions
// Список пользователей, на которых подписан ВЫ.
static lil_value_t tim_tcl_cmd_subscriptions(lil_t lil,
                                             std::size_t argc,
                                             lil_value_t *argv)
{
    (void) argv;

    if (argc != 0)
    {
        lil_set_error(lil,
                      TIM_TR("No arguments expected"_en,
                             "Команда не принимает аргументов"_ru));
        return nullptr;
    }

    tim::tcl *tcl = nullptr;
    tim::prompt_service *prompt = session_or_error(lil, tcl);
    if (!prompt)
        return nullptr;

    tim::sqlite_query q(&prompt->db(),
                        "SELECT u.id, u.nick, u.icon"
                        " FROM subscription s JOIN user u ON s.publisher_id = u.id"
                        " WHERE s.subscriber_id = ?"
                        " ORDER BY u.nick, u.id");
    if (!q.prepare())
    {
        lil_set_error(lil,
                      TIM_TR("Failed to query subscriptions"_en,
                             "Не удалось получить список подписок"_ru));
        return nullptr;
    }
    q.bind(1, prompt->user_id().to_string());

    std::size_t count = 0;
    bool done = false;
    while (q.next(&done) && !done)
    {
        tim::user u;
        u.id = q.to_string(0);
        u.nick = q.to_string(1);
        u.icon = q.to_string(2);
        tcl->terminal()->printf("  %s\n", u.title().c_str());
        ++count;
    }

    if (count == 0)
        tcl->terminal()->printf("%s",
                                TIM_TR("You are not following anyone.\n"_en,
                                       "Вы ни на кого не подписаны.\n"_ru));
    else
        tcl->terminal()->printf(TIM_TR("Total: %zu\n"_en,
                                       "Всего: %zu\n"_ru), count);

    return nullptr;
}

// /subscribers
// Список пользователей, подписанных на ВАС.
static lil_value_t tim_tcl_cmd_subscribers(lil_t lil,
                                           std::size_t argc,
                                           lil_value_t *argv)
{
    (void) argv;

    if (argc != 0)
    {
        lil_set_error(lil,
                      TIM_TR("No arguments expected"_en,
                             "Команда не принимает аргументов"_ru));
        return nullptr;
    }

    tim::tcl *tcl = nullptr;
    tim::prompt_service *prompt = session_or_error(lil, tcl);
    if (!prompt)
        return nullptr;

    tim::sqlite_query q(&prompt->db(),
                        "SELECT u.id, u.nick, u.icon"
                        " FROM subscription s JOIN user u ON s.subscriber_id = u.id"
                        " WHERE s.publisher_id = ?"
                        " ORDER BY u.nick, u.id");
    if (!q.prepare())
    {
        lil_set_error(lil,
                      TIM_TR("Failed to query subscribers"_en,
                             "Не удалось получить список подписчиков"_ru));
        return nullptr;
    }
    q.bind(1, prompt->user_id().to_string());

    std::size_t count = 0;
    bool done = false;
    while (q.next(&done) && !done)
    {
        tim::user u;
        u.id = q.to_string(0);
        u.nick = q.to_string(1);
        u.icon = q.to_string(2);
        tcl->terminal()->printf("  %s\n", u.title().c_str());
        ++count;
    }

    if (count == 0)
        tcl->terminal()->printf("%s",
                                TIM_TR("Nobody is following you.\n"_en,
                                       "На вас никто не подписан.\n"_ru));
    else
        tcl->terminal()->printf(TIM_TR("Total: %zu\n"_en,
                                       "Всего: %zu\n"_ru), count);

    return nullptr;
}


// Public

void tim::prompt::register_user_cmds(lil_t lil)
{
    assert(lil);

    TIM_TCL_REGISTER(lil, setnick);
    TIM_TCL_REGISTER(lil, seticon);
    TIM_TCL_REGISTER(lil, subscribe);
    TIM_TCL_REGISTER(lil, unsubscribe);
    TIM_TCL_REGISTER(lil, subscriptions);
    TIM_TCL_REGISTER(lil, subscribers);
}
