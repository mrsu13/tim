#include "tim_prompt_service.h"

#include "tim_prompt_service_p.h"

#include "tim_mqtt_client.h"
#include "tim_prompt_shell.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_ssh_terminal_protocol.h"
#include "tim_string_tools.h"
#include "tim_tcl.h"
#include "tim_terminal_color_theme.h"
#include "tim_terminal_theme.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_vt.h"

#include <functional>


// Открытые

tim::prompt_service::prompt_service(const tim::ssh_session_info &info, tim::mqtt_client &mqtt, tim::sqlite_db &db)
    : tim::a_ssh_inetd_service("prompt", info)
    , _d(new tim::p::prompt_service(this, mqtt, db))
{
    _d->_user = _d->load_user(info.user_id);
    _d->_proto.reset(new tim::ssh_terminal_protocol(this));
    _d->_terminal.reset(new tim::vt(_d->_proto.get()));
    _d->_tcl.reset(new tim::tcl(_d->_terminal.get(), _d->_user.id, mqtt, db));
    _d->_shell.reset(new tim::prompt_shell(_d->_terminal.get(), _d->_tcl.get()));

    _d->_on_data_ready = _d->_proto->data_ready.connect(
        [d = _d.get()](const char *data, std::size_t size)
        { d->on_data_ready(data, size); });

    _d->_on_posted = _d->_shell->posted.connect(
        [d = _d.get()](const std::string &text)
        {
            if (!d->_mqtt.is_connected())
                return;

            // Каждый пост получает свой UUID и публикуется в
            // post/<publisher-uuid>/<post-uuid>; такой топик однозначно
            // адресует сообщение и используется реакциями.
            const tim::uuid post_uuid = tim::uuid::create();
            const tim::mqtt_topic topic =
                tim::mqtt_topic("post")
                    / d->_user.id.to_string(tim::uuid::format::NoBrackets)
                    / post_uuid.to_string(tim::uuid::format::NoBrackets);
            d->_mqtt.publish(topic, text);
        });

    _d->_on_connected = mqtt.connected.connect(
        [d = _d.get()]{ d->subscribe(); });

    if (mqtt.is_connected())
        _d->subscribe();
}

tim::prompt_service::~prompt_service() = default;

void tim::prompt_service::interrupt() noexcept
{
    // Прерываем активный Tcl-скрипт, если он сейчас выполняется.
    // Вызов lil_break_run в неактивном состоянии "съел" бы следующий
    // eval, поэтому защищаемся проверкой evaluating().
    if (_d->_tcl && _d->_tcl->evaluating())
        _d->_tcl->break_eval();
}


// Закрытые

tim::user tim::p::prompt_service::load_user(const tim::uuid &id)
{
    tim::user u;
    u.id = id;

    tim::sqlite_query q(&_db, "SELECT nick, icon FROM user WHERE id = ?");
    if (!q.prepare())
    {
        TIM_TRACE(Warning,
                  TIM_TR("Failed to prepare query for loading user '%s'."_en,
                         "Не удалось подготовить запрос на загрузку пользователя '%s'."_ru),
                  id.to_string().c_str());
        return u;
    }
    q.bind(1, id.to_string());

    bool done = false;
    if (!q.next(&done) || done)
        return u; // Пользователя ещё нет в БД — отдаём только id.

    u.nick = q.to_string(0);
    u.icon = q.to_string(1);
    return u;
}

tim::user tim::p::prompt_service::user_for(const tim::uuid &id)
{
    if (id == _user.id)
        return _user;

    const std::unordered_map<tim::uuid, tim::user>::iterator it = _known_users.find(id);
    if (it != _known_users.end())
        return it->second;

    tim::user u = load_user(id);
    _known_users.emplace(id, u);
    return u;
}

void tim::p::prompt_service::subscribe()
{
    const std::string user_id_nb = _user.id.to_string(tim::uuid::format::NoBrackets);

    _mqtt.publish("user/connect", _user.id.to_string());

    // Открытый ключ клиента — для аудита и привязки личности к ключу.
    const std::string &key = _q->pub_key();
    if (!key.empty())
        _mqtt.publish(tim::mqtt_topic("user/setpubkey") / user_id_nb, key);

    _sub_post = _mqtt.subscribe("post/+/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_post(topic, data, size); });

    // Слушаем изменения ника/иконки ВСЕХ пользователей — благодаря этому
    // и собственный _user, и кэш других участников остаются актуальными
    // без переподключения и без отдельных запросов в БД.
    _sub_setnick = _mqtt.subscribe("user/setnick/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        {
            const tim::uuid uid = std::string(topic.last_level());
            if (!uid.valid())
                return;
            const std::string nick(data, size);
            if (uid == _user.id)
                _user.nick = nick;
            else
            {
                tim::user &u = _known_users[uid];
                u.id = uid;
                u.nick = nick;
            }
        });

    _sub_seticon = _mqtt.subscribe("user/seticon/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        {
            const tim::uuid uid = std::string(topic.last_level());
            if (!uid.valid())
                return;
            const std::string icon(data, size);
            if (uid == _user.id)
                _user.icon = icon;
            else
            {
                tim::user &u = _known_users[uid];
                u.id = uid;
                u.icon = icon;
            }
        });

    _sub_react_event = _mqtt.subscribe("react_event/+/+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_react_event(topic, data, size); });
}

void tim::p::prompt_service::on_data_ready(const char *data, std::size_t size)
{
    assert(data);

    if (size
            && !_shell->write(data, size))
        _q->close();
}

void tim::p::prompt_service::on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size)
{
    // topic = post/<publisher-uuid>/<post-uuid>
    const std::string post_id_str(topic.last_level());
    const std::string publisher_id_str(topic.parent().last_level());

    const tim::uuid post_id(post_id_str);
    const tim::uuid publisher_id(publisher_id_str);
    if (!post_id.valid() || !publisher_id.valid())
        return;

    // Сохраняем как "последний увиденный" — на него можно реагировать через /react.
    _last_seen_post = post_id;
    _last_seen_post_author = publisher_id;
    _tcl->set_last_post_id(post_id);

    // Свои сообщения — без цвета фона (transparent), заголовок "Me"/"Я".
    // Чужие — заголовок из tim::user::title() и цвет, выведенный из UUID
    // автора, чтобы для одного и того же пользователя цвет был стабильным.
    std::string title;
    tim::color bg_color;
    if (publisher_id == _user.id)
    {
        title = TIM_TR("Me"_en, "Я"_ru);
        bg_color = tim::color::transparent();
    }
    else
    {
        const tim::user sender = user_for(publisher_id);
        title = sender.title();

        const std::size_t color_count = _shell->terminal()->color_count();
        const std::size_t color_idx = color_count > 1
                ? std::hash<std::string>{}(publisher_id_str) % (color_count - 1) + 1
                : 0;
        bg_color = _shell->terminal()->color(color_idx);
    }

    _shell->cloud(title, '\n' + std::string(data, size), bg_color);
    _shell->new_line();
}

void tim::p::prompt_service::on_react_event(const tim::mqtt_topic &topic,
                                            const char *data, std::size_t size)
{
    // topic = react_event/<post-uuid>/<reactor-uuid>
    const tim::uuid reactor_id = std::string(topic.last_level());
    const tim::uuid post_id = std::string(topic.parent().last_level());
    if (!reactor_id.valid() || !post_id.valid())
        return;

    // Не уведомляем о собственной реакции — пользователь и так знает,
    // что нажал /react в этой или другой своей сессии.
    if (reactor_id == _user.id)
        return;

    bool ok = false;
    const int weight = tim::to_int(std::string(data, size), &ok);
    if (!ok)
        return;

    // Узнаём автора поста, чтобы корректно сформулировать "к вашему посту"
    // или "к посту имярек". Если поста нет в БД (например, реакция пришла
    // раньше, чем мы успели его сохранить), молча пропускаем уведомление.
    tim::uuid author_id;
    {
        tim::sqlite_query q(&_db, "SELECT user_id FROM post WHERE id = ?");
        if (!q.prepare())
            return;
        q.bind(1, post_id.to_string());
        bool done = false;
        if (q.next(&done) && !done)
            author_id = q.to_string(0);
    }
    if (!author_id.valid())
        return;

    const tim::user reactor = user_for(reactor_id);
    const bool is_own_post = (author_id == _user.id);

    const tim::color info = _terminal->theme().colors.at(tim::terminal_color_index::Info);
    const tim::color bg = tim::color::transparent();

    if (weight == 0)
    {
        if (is_own_post)
            _terminal->cprintf(info, bg,
                TIM_TR("%s removed reaction from your post.\n"_en,
                       "%s убрал(а) реакцию с вашего поста.\n"_ru),
                reactor.title().c_str());
        else
        {
            const tim::user author = user_for(author_id);
            _terminal->cprintf(info, bg,
                TIM_TR("%s removed reaction from %s's post.\n"_en,
                       "%s убрал(а) реакцию с поста пользователя %s.\n"_ru),
                reactor.title().c_str(), author.title().c_str());
        }
    }
    else
    {
        if (is_own_post)
            _terminal->cprintf(info, bg,
                TIM_TR("%s reacted (%+d) to your post.\n"_en,
                       "%s отреагировал(а) (%+d) на ваш пост.\n"_ru),
                reactor.title().c_str(), weight);
        else
        {
            const tim::user author = user_for(author_id);
            _terminal->cprintf(info, bg,
                TIM_TR("%s reacted (%+d) to %s's post.\n"_en,
                       "%s отреагировал(а) (%+d) на пост пользователя %s.\n"_ru),
                reactor.title().c_str(), weight, author.title().c_str());
        }
    }

    _shell->new_line();
}
