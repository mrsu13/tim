#include "tim_prompt_service.h"

#include "tim_prompt_service_p.h"

#include "tim_mqtt_client.h"
#include "tim_mqtt_topics.h"
#include "tim_prompt_cmd_post.h"
#include "tim_prompt_cmd_user.h"
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

tim::prompt_service::prompt_service(const tim::ssh_session_info &info,
                                    tim::mqtt_client &mqtt,
                                    tim::sqlite_db &db,
                                    std::function<void()> dispatch_handler)
    : tim::a_ssh_inetd_service("prompt", info)
    , _d(new tim::p::prompt_service(this, mqtt, db))
{
    _d->_user = _d->load_user(info.user_id);
    _d->load_subscriptions();
    _d->_proto.reset(new tim::ssh_terminal_protocol(this));
    _d->_terminal.reset(new tim::vt(_d->_proto.get()));
    _d->_tcl.reset(new tim::tcl(_d->_terminal.get()));
    _d->_tcl->set_quit_handler([this]{ close(); });
    _d->_tcl->set_dispatch_handler(std::move(dispatch_handler));
    // Tcl-команды (например /react) достают prompt_service через
    // tcl->user_data() и приводят его обратно к нужному типу.
    _d->_tcl->set_user_data(this);
    // Регистрируем команды чата на стороне владельца — модуль скрипта
    // ничего о messenger-е не знает.
    tim::prompt::register_user_cmds(_d->_tcl->lil());
    tim::prompt::register_post_cmds(_d->_tcl->lil());
    _d->_shell.reset(new tim::prompt_shell(_d->_terminal.get(), _d->_tcl.get()));

    // Печатаем последние сообщения из БД сразу после приглашения,
    // чтобы новый клиент видел контекст разговора.
    _d->load_post_history();

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
            d->_mqtt.publish(tim::topics::post(d->_user.id, post_uuid), text);
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

const tim::uuid &tim::prompt_service::last_seen_post() const noexcept
{
    return _d->_last_seen_post;
}

tim::mqtt_client &tim::prompt_service::mqtt() noexcept
{
    return _d->_mqtt;
}

tim::sqlite_db &tim::prompt_service::db() noexcept
{
    return _d->_db;
}


// Закрытые

tim::user tim::p::prompt_service::load_user(const tim::uuid &id)
{
    tim::user u;
    u.id = id;

    tim::sqlite_query q(&_db, "SELECT nick, icon, motto FROM user WHERE id = ?");
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
    u.motto = q.to_string(2);
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

void tim::p::prompt_service::load_subscriptions()
{
    tim::sqlite_query q(&_db,
                        "SELECT publisher_id FROM subscription WHERE subscriber_id = ?");
    if (!q.prepare())
    {
        TIM_TRACE(Warning,
                  TIM_TR("Failed to prepare query for loading subscriptions of '%s'."_en,
                         "Не удалось подготовить запрос на загрузку подписок '%s'."_ru),
                  _user.id.to_string().c_str());
        return;
    }
    q.bind(1, _user.id.to_string());

    bool done = false;
    while (q.next(&done) && !done)
    {
        const tim::uuid pid = q.to_string(0);
        if (pid.valid())
            _subscriptions.insert(pid);
    }
}

void tim::p::prompt_service::load_post_history()
{
    static const int HISTORY_LIMIT = 20;

    // Берём последние HISTORY_LIMIT сообщений в обратном порядке, затем
    // выводим их в хронологическом — чтобы новые оказались внизу, ближе
    // к приглашению. LEFT JOIN на reaction со сворачиванием по post.id
    // даёт сумму весов и число реакций; для постов без реакций оба
    // значения нулевые.
    tim::sqlite_query q(&_db,
                        "SELECT p.id, p.user_id, p.text,"
                        "       COALESCE(SUM(r.weight), 0) AS rxn_sum,"
                        "       COUNT(r.id) AS rxn_count"
                        " FROM post p LEFT JOIN reaction r ON r.post_id = p.id"
                        " GROUP BY p.id"
                        " ORDER BY p.timestamp DESC LIMIT ?");
    if (!q.prepare())
    {
        TIM_TRACE(Warning, "%s",
                  TIM_TR("Failed to prepare query for loading post history."_en,
                         "Не удалось подготовить запрос на загрузку истории сообщений."_ru));
        return;
    }
    q.bind(1, HISTORY_LIMIT);

    struct entry
    {
        tim::uuid     id;
        tim::uuid     author;
        std::string   text;
        int           rxn_sum = 0;
        std::size_t   rxn_count = 0;
    };
    std::vector<entry> history;

    bool done = false;
    while (q.next(&done) && !done)
    {
        entry e;
        e.id = q.to_string(0);
        e.author = q.to_string(1);
        e.text = q.to_string(2);
        e.rxn_sum = q.to_int(3);
        e.rxn_count = static_cast<std::size_t>(q.to_int(4));
        if (e.id.valid() && e.author.valid())
            history.push_back(std::move(e));
    }

    if (history.empty())
        return;

    const tim::color info_color = _terminal->theme().colors.at(tim::terminal_color_index::Info);
    const tim::color transparent = tim::color::transparent();

    _shell->hide_input();
    for (std::vector<entry>::reverse_iterator it = history.rbegin();
         it != history.rend(); ++it)
    {
        render_post(it->author, it->text);
        if (it->rxn_count > 0)
            _terminal->cprintf(info_color, transparent,
                               "  %+d (%zu)\n",
                               it->rxn_sum, it->rxn_count);
        _last_seen_post = it->id;
        _last_seen_post_author = it->author;
    }
    _shell->new_line();
    _shell->show_input();
}

void tim::p::prompt_service::subscribe()
{
    // Этот метод вызывается на каждом подключении и повторном подключении
    // к брокеру. После повторного подключения локальный кэш _subscriptions
    // мог разойтись с БД: события user/subscribe и user/unsubscribe,
    // пришедшие в окно отказа брокера, мы пропустили (даже при QoS=1 —
    // сессия клиента не была персистентной). Перезагружаем кэш из БД,
    // чтобы метка подписки (звёздочка перед автором) снова отражала
    // фактическое состояние.
    _subscriptions.clear();
    load_subscriptions();

    _mqtt.publish(tim::topics::USER_CONNECT, _user.id.to_string());

    // Открытый ключ клиента — для аудита и привязки личности к ключу.
    const std::string &key = _q->pub_key();
    if (!key.empty())
        _mqtt.publish(tim::topics::user_setpubkey(_user.id), key);

    _sub_post = _mqtt.subscribe(tim::topics::POST_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_post(topic, data, size); });

    // Слушаем изменения ника/иконки ВСЕХ пользователей — благодаря этому
    // и собственный _user, и кэш других участников остаются актуальными
    // без переподключения и без отдельных запросов в БД.
    _sub_setnick = _mqtt.subscribe(tim::topics::USER_SETNICK_FILTER,
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

    _sub_seticon = _mqtt.subscribe(tim::topics::USER_SETICON_FILTER,
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

    _sub_setmotto = _mqtt.subscribe(tim::topics::USER_SETMOTTO_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        {
            const tim::uuid uid = std::string(topic.last_level());
            if (!uid.valid())
                return;
            const std::string motto(data, size);
            if (uid == _user.id)
                _user.motto = motto;
            else
            {
                tim::user &u = _known_users[uid];
                u.id = uid;
                u.motto = motto;
            }
        });

    _sub_react_event = _mqtt.subscribe(tim::topics::REACT_EVENT_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_react_event(topic, data, size); });

    // Свои подписки/отписки — синхронизируем локальный кэш _subscriptions
    // в реальном времени без перезагрузки из БД. Wildcard на всех
    // пользователей здесь не нужен (нас интересует только свой список).
    _sub_self_subscribe = _mqtt.subscribe(tim::topics::user_subscribe(_user.id),
        [this](const tim::mqtt_topic &, const char *data, std::size_t size)
        {
            const tim::uuid pub = std::string(data, size);
            if (pub.valid())
                _subscriptions.insert(pub);
        });

    _sub_self_unsubscribe = _mqtt.subscribe(tim::topics::user_unsubscribe(_user.id),
        [this](const tim::mqtt_topic &, const char *data, std::size_t size)
        {
            const tim::uuid pub = std::string(data, size);
            if (pub.valid())
                _subscriptions.erase(pub);
        });

    // Личные уведомления (ошибки сервера для своих действий — "ник занят" и т.п.).
    _sub_notice = _mqtt.subscribe(tim::topics::session_notice(_user.id),
        [this](const tim::mqtt_topic &, const char *data, std::size_t size)
        {
            // Прячем строку ввода на время вывода: иначе уведомление
            // ломает то, что пользователь набирает в этот момент.
            _shell->hide_input();
            const tim::color warning = _terminal->theme().colors.at(tim::terminal_color_index::Warning);
            _terminal->cprintf(warning, tim::color::transparent(),
                               "! %.*s\n", (int)size, data);
            _shell->new_line();
            _shell->show_input();
        });
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

    _shell->hide_input();
    render_post(publisher_id, std::string_view(data, size));
    _shell->new_line();
    _shell->show_input();
}

void tim::p::prompt_service::render_post(const tim::uuid &publisher_id, std::string_view text)
{
    // Свои сообщения — без цвета фона (transparent), заголовок "Me"/"Я".
    // Чужие — заголовок из tim::user::title() и цвет, выведенный из UUID
    // автора, чтобы для одного и того же пользователя цвет был стабильным.
    std::string title;
    tim::color bg_color;
    tim::color marker_color; // пустой по умолчанию — звёздочки не будет
    if (publisher_id == _user.id)
    {
        title = TIM_TR("Me"_en, "Я"_ru);
        bg_color = tim::color::transparent();
    }
    else
    {
        const tim::user sender = user_for(publisher_id);
        title = sender.title();

        const std::string publisher_id_str = publisher_id.to_string(tim::uuid::format::NoBrackets);
        const std::size_t color_count = _shell->terminal()->color_count();
        const std::size_t color_idx = color_count > 1
                ? std::hash<std::string>{}(publisher_id_str) % (color_count - 1) + 1
                : 0;
        bg_color = _shell->terminal()->color(color_idx);

        // Жёлтая звезда для сообщений от тех, на кого мы подписаны.
        if (_subscriptions.find(publisher_id) != _subscriptions.end())
            marker_color = tim::color{ 255, 255, 0 };
    }

    _shell->cloud(title, '\n' + std::string(text), bg_color, marker_color);
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

    _shell->hide_input();
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
    _shell->show_input();
}
