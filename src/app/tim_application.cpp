#include "tim_application.h"

#include "tim_application_p.h"

#include "tim_config.h"
#include "tim_file_tools.h"
#include "tim_inetd.h"
#include "tim_mqtt_client.h"
#include "tim_sqlite_db.h"
#include "tim_trace.h"
#include "tim_version.h"

#include "fort.h"

// Services
#include "tim_a_inetd_service.h"
#include "tim_post_service.h"
#include "tim_user_service.h"
#include "tim_prompt_service.h"

#include <cassert>
#include <cstring>
#include <locale>


tim::application *tim::app()
{
    return tim::p::application::instance();
}


// Public

tim::application::application(int argc, char **argv)
    : _d(new tim::p::application())
{
    // FIXME!
    (void) argc;
    (void) argv;

    assert(!tim::p::application::instance() && "tim::application instantiated already.");

    tim::p::application::instance() = this;

    std::setbuf(stdout, nullptr);
    std::setbuf(stderr, nullptr);

#ifdef TIM_OS_LINUX
    std::locale::global(std::locale("en_US.UTF-8"));

    {
        struct sigaction action;
        std::memset(&action, 0, sizeof(action));
        action.sa_handler = &tim::p::application::signal_handler;
        action.sa_flags = 0;
        sigemptyset(&action.sa_mask);
        sigaction(SIGINT, &action, &_d->_old_sig_int);
        sigaction(SIGTERM, &action, &_d->_old_sig_term);
    }
#endif

    ft_set_default_border_style(FT_SOLID_ROUND_STYLE);

#ifdef TIM_DEBUG
    // mg_log_set(MG_LL_VERBOSE);
#endif
    mg_mgr_init(&_d->_mg);

    _d->_mqtt.reset(new tim::mqtt_client(&_d->_mg));
    if (!_d->_mqtt->start())
        TIM_TRACE(Error, "%s",
                  TIM_TR("MQTT client failed to start; messaging will be unavailable."_en,
                         "Не удалось запустить MQTT-клиент; обмен сообщениями недоступен."_ru));

    _d->_db.reset(new tim::sqlite_db());
    if (!_d->_db->open(tim::standard_location(tim::filesystem_location::AppLocalData)
                                                   / tim::DB_FILE_NAME))
        TIM_TRACE(Error,
                  TIM_TR("Failed to open database file '%s'; persistence will be unavailable."_en,
                         "Не удалось открыть файл базы данных '%s'; хранение данных недоступно."_ru),
                  _d->_db->path().string().c_str());

    _d->_prompt_inetd = tim::inetd::start(
        &_d->_mg, tim::TELNET_PORT,
        [mqtt = _d->_mqtt.get()](mg_connection *c) -> std::unique_ptr<tim::a_inetd_service>
        {
            return std::make_unique<tim::prompt_service>(c, *mqtt);
        },
        /*tls_enabled=*/false);
    if (!_d->_prompt_inetd)
        TIM_TRACE(Error,
                  TIM_TR("Telnet inetd failed to start on port %u; clients cannot connect."_en,
                         "Не удалось запустить telnet-inetd на порту %u; клиенты не смогут подключиться."_ru),
                  tim::TELNET_PORT);

    _d->_post_service.reset(new tim::post_service(*_d->_mqtt, *_d->_db));
    _d->_user_service.reset(new tim::user_service(*_d->_mqtt, *_d->_db));
}

tim::application::~application()
{
    mg_mgr_free(&_d->_mg);

#ifdef TIM_OS_LINUX
    sigaction(SIGINT, &_d->_old_sig_int, nullptr);
    sigaction(SIGTERM, &_d->_old_sig_term, nullptr);
#endif
}

const std::string &tim::application::name()
{
    return tim::p::application::name();
}

void tim::application::set_name(const std::string &name)
{
    tim::p::application::name() = name;
}

const std::string &tim::application::org_name()
{
    return tim::p::application::org_name();
}

void tim::application::set_org_name(const std::string &name)
{
    tim::p::application::org_name() = name;
}

void tim::application::dispatch()
{
    mg_mgr_poll(&_d->_mg, 0);
}

void tim::application::exec()
{
    while (!_d->_quit)
        mg_mgr_poll(&_d->_mg, 1000 /* 1 sec */);
}

void tim::application::quit()
{
    _d->_quit = true;
}

mg_mgr *tim::application::mongoose() const
{
    return &_d->_mg;
}

tim::mqtt_client *tim::application::mqtt() const
{
    return _d->_mqtt.get();
}

tim::sqlite_db *tim::application::db() const
{
    return _d->_db.get();
}


// Private

#ifdef TIM_OS_LINUX

void tim::p::application::signal_handler(int sig_num)
{
    switch (sig_num)
    {
        case SIGTERM:
        case SIGINT:
            if (tim::p::application::instance())
            {
                TIM_TRACE(Debug, "Exiting on signal %d ...", sig_num);
                tim::p::application::instance()->quit();
            }
            break;

        default:
            break;
    }
}

#endif
