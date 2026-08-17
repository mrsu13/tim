#include "tim_prompt_service.h"

#include "tim_prompt_service_p.h"

#include "tim_config.h"
#include "tim_mqtt_client.h"
#include "tim_mqtt_topics.h"
#include "tim_prompt_cmd_post.h"
#include "tim_prompt_cmd_user.h"
#include "tim_prompt_shell.h"
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

/**
 * Конструктор. Поднимает терминал, Tcl и подписки.
 *
 * \param info Параметры SSH-сессии (terminal, user_id, размеры).
 * \param mqtt MQTT-клиент приложения; должен жить дольше сессии.
 * \param db Подключение к БД; должно жить дольше сессии.
 * \param dispatch_handler Обработчик, который Tcl-интерпретатор будет
 *                         вызывать между операторами для продвижения
 *                         внешних циклов событий. Обычно
 *                         application::dispatch().
 */
tim::prompt_service::prompt_service(const tim::ssh_session_info &info,
                                    tim::mqtt_client &mqtt,
                                    tim::sqlite_db &db,
                                    std::function<void()> dispatch_handler)
    : tim::a_ssh_inetd_service("prompt", info)
    , _d(this, mqtt, db, info.user_id)
{
    _d->load_subscriptions();
    _d->_proto.reset(new tim::ssh_terminal_protocol(this));
    _d->_terminal.reset(new tim::vt(_d->_proto.get()));
    _d->_tcl.reset(new tim::tcl(_d->_terminal.get()));
    // /quit закрывает только эту сессию, не процесс.
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

    // Байты из SSH-протокола → on_data_ready → командный интерпретатор.
    _d->_on_data_ready = _d->_proto->data_ready.connect(
        [d = _d.get()](const char *data, std::size_t size)
        { d->on_data_ready(data, size); });

    // Введённый текст, не являющийся командой, интерпретатор испускает
    // сигналом posted; мы публикуем его в MQTT под новым UUID сообщения.
    _d->_on_posted = _d->_shell->posted.connect(
        [d = _d.get()](const std::string &text)
        {
            // Каждый пост получает свой UUID и публикуется в
            // post/<publisher-uuid>/<post-uuid>; такой топик однозначно
            // адресует сообщение и используется реакциями.
            const tim::uuid post_uuid = tim::uuid::create();
            d->_mqtt.publish(tim::topics::post(d->_profiles.self().id, post_uuid), text);

            // Без соединения публикация ушла в очередь mqtt_client
            // и будет отправлена после восстановления соединения;
            // предупреждаем пользователя о задержке доставки.
            if (!d->_mqtt.is_connected())
                d->notice(TIM_TR("No broker connection; the message will be delivered "
                                 "after the connection is restored."_en,
                                 "Нет соединения с брокером; сообщение будет доставлено "
                                 "после восстановления соединения."_ru));
        });

    // Подписки сессии регистрируются однократно: их оформление у брокера
    // при каждом восстановлении соединения mqtt_client выполняет сам.
    _d->subscribe();

    // Домен реагирует на восстановление соединения отдельно: кэши могли
    // разойтись с реальным состоянием за время разрыва.
    _d->_on_connected = mqtt.connected.connect(
        [d = _d.get()]{ d->on_broker_connected(); });

    if (mqtt.is_connected())
        _d->on_broker_connected();
}

/** Деструктор. Останавливает Tcl, рвёт подписки. */
tim::prompt_service::~prompt_service() = default;

/**
 * Прерывает активный Tcl-скрипт сессии. Вызывается ssh_inetd на
 * завершении сервера, чтобы /while 1 {} не блокировал выход.
 */
void tim::prompt_service::interrupt() noexcept
{
    // Прерываем активный Tcl-скрипт, если он сейчас выполняется.
    // Вызов lil_break_run в неактивном состоянии "съел" бы следующий
    // eval, поэтому защищаемся проверкой evaluating().
    if (_d->_tcl && _d->_tcl->evaluating())
        _d->_tcl->break_eval();
}

/**
 * \return UUID последнего сообщения, увиденного в этой сессии
 *         (от другого пользователя или из истории). Команда /react
 *         использует его как неявную цель.
 */
const tim::uuid &tim::prompt_service::last_seen_post() const noexcept
{
    return _d->_last_seen_post;
}

/**
 * \return MQTT-клиент сессии. Доступ для Tcl-команд через
 *         tcl->user_data() → prompt_service.
 */
tim::mqtt_client &tim::prompt_service::mqtt() noexcept
{
    return _d->_mqtt;
}

/**
 * \return Подключение к БД сессии. Доступ для Tcl-команд через
 *         tcl->user_data() → prompt_service.
 */
tim::sqlite_db &tim::prompt_service::db() noexcept
{
    return _d->_db;
}


// Закрытые

/**
 * Подтягивает текущие подписки пользователя из таблицы subscription
 * в кэш _subscriptions (UUID-ы тех, на кого подписан текущий
 * пользователь — для пометки сообщений звёздочкой при отрисовке).
 */
void tim::p::prompt_service::load_subscriptions()
{
    tim::sqlite_query q(&_db,
                        "SELECT publisher_id FROM subscription WHERE subscriber_id = ?");
    if (!q.prepare())
    {
        TIM_TRACE(warning,
                  TIM_TR("Failed to prepare query for loading subscriptions of '%s'."_en,
                         "Не удалось подготовить запрос на загрузку подписок '%s'."_ru),
                  _profiles.self().id.to_string().c_str());
        return;
    }
    q.bind(1, _profiles.self().id.to_string());

    bool done = false;
    while (q.next(&done) && !done)
    {
        const tim::uuid pid = q.to_string(0);
        if (pid.valid())
            _subscriptions.insert(pid);
    }
}

/**
 * Загружает последние tim::POST_HISTORY_LIMIT сообщений из БД (с суммарным
 * весом и числом реакций) и печатает их в терминал в хронологическом
 * порядке — чтобы пользователь, только зашедший в чат, сразу видел
 * контекст разговора.
 */
void tim::p::prompt_service::load_post_history()
{
    // Берём последние HISTORY_LIMIT сообщений в обратном порядке, затем
    // выводим их в хронологическом — чтобы новые оказались внизу, ближе
    // к приглашению. LEFT JOIN на reaction с группировкой по post.id
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
        TIM_TRACE(warning, "%s",
                  TIM_TR("Failed to prepare query for loading post history."_en,
                         "Не удалось подготовить запрос на загрузку истории сообщений."_ru));
        return;
    }
    q.bind(1, tim::POST_HISTORY_LIMIT);

    /** Одна запись истории: post + агрегаты реакций. */
    struct entry
    {
        tim::uuid     id;            ///< UUID сообщения.
        tim::uuid     author;        ///< UUID автора.
        std::string   text;          ///< Текст сообщения.
        int           rxn_sum = 0;   ///< Сумма весов реакций (0 — нет реакций).
        std::size_t   rxn_count = 0; ///< Число реакций (0 — нет реакций).
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

    const tim::color info_color = _terminal->theme().colors.at(tim::terminal_color_index::info);
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

/**
 * Регистрирует все MQTT-подписки сессии. Вызывается однократно из
 * конструктора: оформление подписок у брокера — в том числе после
 * каждого восстановления соединения — mqtt_client выполняет сам.
 */
void tim::p::prompt_service::subscribe()
{
    const tim::uuid &self_id = _profiles.self().id;

    // Подписка на все посты: выводим карточку каждого входящего.
    _sub_post = _mqtt.subscribe(tim::topics::POST_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_post(topic, data, size); });

    // Подтверждённые реакции: показываем "X отреагировал(а) на ваш пост".
    _sub_react_event = _mqtt.subscribe(tim::topics::REACT_EVENT_FILTER,
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_react_event(topic, data, size); });

    // Свои подписки/отписки — синхронизируем локальный кэш _subscriptions
    // в реальном времени без перезагрузки из БД. Wildcard на всех
    // пользователей здесь не нужен (нас интересует только свой список).
    _sub_self_subscribe = _mqtt.subscribe(tim::topics::user_subscribe(self_id),
        [this](const tim::mqtt_topic &, const char *data, std::size_t size)
        {
            // Payload — UUID издателя, на которого подписался текущий
            // пользователь; добавляем в кэш помеченных подпиской.
            const tim::uuid pub = std::string(data, size);
            if (pub.valid())
                _subscriptions.insert(pub);
        });

    _sub_self_unsubscribe = _mqtt.subscribe(tim::topics::user_unsubscribe(self_id),
        [this](const tim::mqtt_topic &, const char *data, std::size_t size)
        {
            // Payload — UUID издателя, от которого отписался текущий
            // пользователь; удаляем из кэша.
            const tim::uuid pub = std::string(data, size);
            if (pub.valid())
                _subscriptions.erase(pub);
        });

    // Личные уведомления (ошибки сервера для своих действий — "ник занят" и т.п.).
    _sub_notice = _mqtt.subscribe(tim::topics::session_notice(self_id),
        [this](const tim::mqtt_topic &, const char *data, std::size_t size)
        { notice(std::string(data, size)); });
}

/**
 * Обработчик восстановления соединения с брокером. Локальные кэши могли
 * разойтись с реальным состоянием: события setnick/seticon/setmotto и
 * user/subscribe|unsubscribe, пришедшие в окно разрыва, утрачены (сессия
 * клиента у брокера не персистентна). Сбрасываем кэш профилей,
 * перечитываем подписки из БД и повторно объявляем присутствие
 * пользователя.
 */
void tim::p::prompt_service::on_broker_connected()
{
    _profiles.invalidate();

    _subscriptions.clear();
    load_subscriptions();

    const tim::uuid &self_id = _profiles.self().id;
    _mqtt.publish(tim::topics::USER_CONNECT, self_id.to_string());

    // Открытый ключ клиента — для аудита и привязки личности к ключу.
    const std::string &key = _q->pub_key();
    if (!key.empty())
        _mqtt.publish(tim::topics::user_setpubkey(self_id), key);
}

/**
 * Выводит служебное уведомление сессии поверх строки ввода
 * (цветом warning текущей темы).
 *
 * \param text Текст уведомления.
 */
void tim::p::prompt_service::notice(const std::string &text)
{
    // Прячем строку ввода на время вывода: иначе уведомление ломает то,
    // что пользователь набирает в этот момент.
    _shell->hide_input();
    const tim::color warning = _terminal->theme().colors.at(tim::terminal_color_index::warning);
    _terminal->cprintf(warning, tim::color::transparent(),
                       "! %s\n", text.c_str());
    _shell->new_line();
    _shell->show_input();
}

/**
 * Передаёт сырые байты ввода командному интерпретатору. При ошибке
 * записи (Ctrl+D или сбой ввода-вывода) закрывает соединение.
 */
void tim::p::prompt_service::on_data_ready(const char *data, std::size_t size)
{
    assert(data);

    if (size
            && !_shell->write(data, size))
        _q->close();
}

/**
 * Обработчик POST_FILTER: разбирает UUID из топика, сохраняет пост как
 * последний увиденный (для /react) и выводит карточку сообщения
 * через render_post().
 */
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

/**
 * Выводит карточку одного сообщения. Для своих сообщений — без фона,
 * заголовок Me/Я. Для чужих цвет фона детерминированно выводится из
 * UUID автора (одинаковый автор → одинаковый цвет), а если автор есть
 * в _subscriptions, в карточке появляется жёлтая звёздочка.
 *
 * \param publisher_id UUID автора.
 * \param text Текст сообщения.
 */
void tim::p::prompt_service::render_post(const tim::uuid &publisher_id, std::string_view text)
{
    // Свои сообщения — без цвета фона (transparent), заголовок "Me"/"Я".
    // Чужие — заголовок из tim::user::title() и цвет, выведенный из UUID
    // автора, чтобы для одного и того же пользователя цвет был стабильным.
    std::string title;
    tim::color bg_color;
    tim::color marker_color; // пустой по умолчанию — звёздочки не будет
    if (publisher_id == _profiles.self().id)
    {
        title = TIM_TR("Me"_en, "Я"_ru);
        bg_color = tim::color::transparent();
    }
    else
    {
        const tim::user sender = _profiles.user_for(publisher_id);
        title = sender.title();

        const std::string publisher_id_str = publisher_id.to_string(tim::uuid::format::no_brackets);
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

/**
 * Обработчик REACT_EVENT_FILTER: парсит UUID-ы реактора и поста, узнаёт
 * автора поста из БД, и выводит локализованное "X отреагировал(а)/убрал(а)
 * реакцию на ваш пост / на пост Y" — с учётом того, что собственные
 * реакции мы не показываем.
 */
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
    if (reactor_id == _profiles.self().id)
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

    const tim::user reactor = _profiles.user_for(reactor_id);
    const bool is_own_post = (author_id == _profiles.self().id);

    const tim::color info = _terminal->theme().colors.at(tim::terminal_color_index::info);
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
            const tim::user author = _profiles.user_for(author_id);
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
            const tim::user author = _profiles.user_for(author_id);
            _terminal->cprintf(info, bg,
                TIM_TR("%s reacted (%+d) to %s's post.\n"_en,
                       "%s отреагировал(а) (%+d) на пост пользователя %s.\n"_ru),
                reactor.title().c_str(), weight, author.title().c_str());
        }
    }

    _shell->new_line();
    _shell->show_input();
}
