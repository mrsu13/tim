#include "tim_prompt_service.h"

#include "tim_prompt_service_p.h"

#include "tim_mqtt_client.h"
#include "tim_prompt_shell.h"
#include "tim_sqlite_db.h"
#include "tim_sqlite_query.h"
#include "tim_ssh_terminal_protocol.h"
#include "tim_string_tools.h"
#include "tim_tcl.h"
#include "tim_trace.h"
#include "tim_translator.h"
#include "tim_vt.h"

#include <functional>


// Открытые

tim::prompt_service::prompt_service(const tim::ssh_session_info &info, tim::mqtt_client &mqtt, tim::sqlite_db &db)
    : tim::a_ssh_inetd_service("prompt", info)
    , _d(new tim::p::prompt_service(this, mqtt, db))
{
    _d->_user.id = info.user_id;
    _d->load_user_from_db();
    _d->_proto.reset(new tim::ssh_terminal_protocol(this));
    _d->_terminal.reset(new tim::vt(_d->_proto.get()));
    _d->_tcl.reset(new tim::tcl(_d->_terminal.get(), _d->_user.id, mqtt));
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


// Закрытые

void tim::p::prompt_service::load_user_from_db()
{
    // Подтягиваем ранее сохранённые ник и иконку из БД, чтобы при повторном
    // подключении title() показывал именно их, а не fallback по UUID.
    tim::sqlite_query q(&_db, "SELECT nick, icon FROM user WHERE id = ?");
    if (!q.prepare())
    {
        TIM_TRACE(Warning,
                  TIM_TR("Failed to prepare query for loading user '%s'."_en,
                         "Не удалось подготовить запрос на загрузку пользователя '%s'."_ru),
                  _user.id.to_string().c_str());
        return;
    }
    q.bind(1, _user.id.to_string());

    bool done = false;
    if (!q.next(&done) || done)
        return; // Пользователь ещё не существует в БД — оставляем пустые поля.

    _user.nick = q.to_string(0);
    _user.icon = q.to_string(1);
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

    // Подписываемся ровно на свои события изменения ника/иконки — никнейм
    // и иконку в текущей сессии обновляем без переподключения. Использовать
    // здесь общий wildcard '+' было бы расточительно: брокер прислал бы
    // нам события всех пользователей просто чтобы мы их отбросили.
    _sub_self_setnick = _mqtt.subscribe(tim::mqtt_topic("user/setnick") / user_id_nb,
        [this](const tim::mqtt_topic &, const char *data, std::size_t size)
        { _user.nick.assign(data, size); });

    _sub_self_seticon = _mqtt.subscribe(tim::mqtt_topic("user/seticon") / user_id_nb,
        [this](const tim::mqtt_topic &, const char *data, std::size_t size)
        { _user.icon.assign(data, size); });
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
        tim::user sender;
        sender.id = publisher_id;
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
