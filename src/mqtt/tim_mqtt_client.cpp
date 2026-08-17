#include "tim_mqtt_client.h"

#include "tim_mqtt_client_p.h"

#include "tim_application.h"
#include "tim_config.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include "mongoose.h"

#include <algorithm>
#include <cassert>


/** QoS, с которым шлются keep-alive публикации client/status. */
static const int TIM_MQTT_QOS = 1;

// Открытые

/**
 * Конструктор. Не делает сетевых операций; реальное соединение
 * с брокером открывается в start().
 *
 * \param mg Mongoose-менеджер, владелец таймеров и соединений.
 *           Должен жить дольше mqtt_client.
 */
tim::mqtt_client::mqtt_client(mg_mgr *mg)
    : connected{}
    , disconnected{}
    , _d(this)
{
    assert(mg);
    _d->_mg = mg;
}

/** Деструктор; вызывает stop() при необходимости. */
tim::mqtt_client::~mqtt_client()
{
    stop();
}

/**
 * Запускает периодический таймер для подключения и keep-alive:
 * при первом срабатывании он выполнит попытку подключения к брокеру,
 * при последующих — будет отправлять keep-alive PING. Сетевые ошибки и
 * недоступность брокера не считаются синхронными ошибками —
 * таймер повторит попытку при следующем срабатывании.
 *
 * \param url URL брокера: mqtts:// (TLS) или mqtt:// (без TLS).
 * \param ping_interval Период keep-alive PING.
 * \return false при синхронной ошибке (например, не удалось создать
 *         таймер); true при успешной постановке таймера.
 */
bool tim::mqtt_client::start(std::string_view url, const std::chrono::seconds ping_interval)
{
    assert(!url.empty() && "MQTT broker URL must not be empty.");
    assert(ping_interval.count() && "ping_interval must not be zero.");

    if (_d->_timer)
    {
        TIM_TRACE(warning, "mqtt_client::start() called while already started; ignoring.");
        return true;
    }

    _d->_url = url;
    _d->_timer = mg_timer_add(_d->_mg, ping_interval.count() * 1000,
                              MG_TIMER_REPEAT | MG_TIMER_RUN_NOW,
                              &tim::p::mqtt_client::ping, _d.get());
    if (!_d->_timer)
    {
        TIM_TRACE(error,
                  TIM_TR("Failed to start MQTT client timer for broker '%.*s'."_en,
                         "Не могу создать таймер MQTT клиента для брокера '%.*s'."_ru),
                  (int)url.size(), url.data());
        _d->_url.clear();
        return false;
    }
    return true;
}

/**
 * Останавливает таймер и закрывает текущее соединение. Идемпотентно:
 * повторный вызов на остановленном клиенте — no-op.
 */
void tim::mqtt_client::stop()
{
    if (_d->_timer)
    {
        mg_timer_free(&_d->_mg->timers, _d->_timer);
        _d->_timer = nullptr;
    }
    if (_d->_client)
    {
        // Отвязываем обработчики от соединения: mg_mgr_free вызовет MG_EV_CLOSE
        // на всех открытых соединениях, и без этого обработчик попытался бы
        // прочитать уже освобождённый _d (use-after-free, проявляющийся
        // некорректной записью в журнале вида "broker '<garbage>' closed").
        _d->_client->fn_data = nullptr;
        _d->_client->is_draining = 1;
        _d->_client = nullptr;
    }
    _d->_connected = false;
    _d->_url.clear();
}

/** \return true, если MQTT-рукопожатие завершилось и соединение активно. */
bool tim::mqtt_client::is_connected() const
{
    return _d->_connected;
}

/**
 * Публикует сообщение в топик.
 *
 * Публикация с QoS > 0 помещается в очередь неподтверждённых (outbox)
 * до прихода PUBACK; при отсутствии соединения она будет отправлена
 * после его установления, при обрыве до подтверждения — отправлена
 * повторно с тем же packet id (флаг DUP). Публикация с QoS 0 при
 * отсутствии соединения отбрасывается.
 *
 * \param topic MQTT-топик публикации.
 * \param data Указатель на полезную нагрузку.
 * \param size Размер полезной нагрузки в байтах.
 * \param qos QoS уровень (0/1/2). По умолчанию 1.
 * \param retain Retain-флаг (брокер запомнит последнее сообщение).
 * \return true, если публикация отправлена или поставлена в очередь;
 *         false, если отброшена (QoS 0 без соединения либо переполнение
 *         очереди).
 */
bool tim::mqtt_client::publish(const tim::mqtt_topic &topic,
                               const char *data, std::size_t size,
                               std::uint8_t qos,
                               bool retain)
{
    assert(!topic.empty() && "Topic must not be empty.");

    if (!qos)
    {
        // QoS 0 — принципиально без гарантий: без соединения просто
        // отбрасываем, с соединением отправляем без учёта в outbox.
        if (!_d->_client || !_d->_connected)
            return false;

        const mg_mqtt_opts pub_opts =
        {
            .topic = mg_str_n(topic.data(), topic.size()),
            .message = mg_str_n(data, size),
            .qos = qos,
            .retain = retain
        };
        mg_mqtt_pub(_d->_client, &pub_opts);

        TIM_TRACE(debug, "Published to '%s': '%.*s'.",
                  topic.c_str(),
                  (int)size, data);
        return true;
    }

    // QoS > 0 — до подтверждения PUBACK запись хранится в outbox.
    if (_d->_outbox.size() >= tim::MQTT_OUTBOX_LIMIT)
    {
        // Отбрасываем самую старую неподтверждённую публикацию: свежие
        // данные для чата ценнее давно ожидающих.
        TIM_TRACE(warning,
                  TIM_TR("MQTT outbox overflow (%zu entries); dropping the oldest publication."_en,
                         "Переполнение очереди MQTT-публикаций (%zu записей); самая старая отброшена."_ru),
                  _d->_outbox.size());
        _d->_outbox.erase(_d->_outbox.begin());
    }

    tim::p::mqtt_client::outbox_entry entry =
    {
        .topic = topic,
        .payload = std::string(data, size),
        .qos = qos,
        .retain = retain,
        .packet_id = 0
    };

    if (_d->_client && _d->_connected)
    {
        const mg_mqtt_opts pub_opts =
        {
            .topic = mg_str_n(topic.data(), topic.size()),
            .message = mg_str_n(data, size),
            .qos = qos,
            .retain = retain
        };
        entry.packet_id = mg_mqtt_pub(_d->_client, &pub_opts);

        TIM_TRACE(debug, "Published to '%s': '%.*s'.",
                  topic.c_str(),
                  (int)size, data);
    }
    else
        TIM_TRACE(debug, "Queued publication to '%s' until the broker connection is restored.",
                  topic.c_str());

    _d->_outbox.push_back(std::move(entry));
    return true;
}

/**
 * Удобный перегруз publish() для string_view нагрузки; делегирует
 * на основной вариант.
 *
 * \param topic MQTT-топик публикации.
 * \param payload Полезная нагрузка.
 * \param qos QoS уровень. По умолчанию 1.
 * \param retain Retain-флаг.
 * \return См. основной publish().
 */
bool tim::mqtt_client::publish(const tim::mqtt_topic &topic,
                               std::string_view payload,
                               std::uint8_t qos,
                               bool retain)
{
    return publish(topic, payload.data(), payload.size(), qos, retain);
}

/**
 * Регистрирует подписчика в списке клиента и, при наличии соединения,
 * оформляет подписку у брокера. При отсутствии соединения (и при каждом
 * его восстановлении) подписка будет оформлена автоматически в момент
 * MQTT-рукопожатия — повторный вызов subscribe() не требуется.
 *
 * \param topic_filter Фильтр (поддерживаются + и #).
 * \param mh Обработчик входящих сообщений, удовлетворяющих фильтру.
 * \param qos QoS уровень подписки. По умолчанию 1.
 * \return RAII-объект подписки; при разрушении автоматически
 *         отписывается. [[nodiscard]] — несохранённый объект недопустим.
 */
tim::mqtt_subscription tim::mqtt_client::subscribe(const tim::mqtt_topic &topic_filter,
                                                   message_handler mh,
                                                   std::uint8_t qos)
{
    assert(!topic_filter.empty() && "Topic filter must not be empty.");
    assert(mh);

    const std::size_t id = _d->_next_subscriber_id++;
    _d->_subscribers.push_back({ id, topic_filter, std::move(mh), qos });

    if (_d->_client && _d->_connected)
    {
        const mg_mqtt_opts sub_opts =
        {
            .topic = mg_str_n(topic_filter.data(), topic_filter.size()),
            .qos = qos
        };
        mg_mqtt_sub(_d->_client, &sub_opts);
    }

    TIM_TRACE(debug, "Subscribed to '%s'.", topic_filter.c_str());

    return tim::mqtt_subscription(_d->_alive, id);
}

/**
 * Снимает подписку по идентификатору: удаляет подписчика из списка
 * по id. Вызывается деструктором mqtt_subscription; на остановленном
 * клиенте безопасен.
 *
 * \param id Идентификатор подписки.
 */
void tim::mqtt_client::unsubscribe(std::size_t id)
{
    // Линейный поиск по id; список подписчиков обычно невелик.
    const tim::p::mqtt_client::subscribers::iterator it = std::find_if(
        _d->_subscribers.begin(), _d->_subscribers.end(),
        [id](const tim::p::mqtt_client::subscriber_entry &e){ return e.id == id; });
    if (it == _d->_subscribers.end())
        return;

    TIM_TRACE(debug, "Unsubscribed from '%s'.", it->filter.c_str());
    _d->_subscribers.erase(it);
}


// Закрытые

/**
 * Mongoose-обработчик событий соединения. Маршрутизирует MG_EV_*
 * по веткам switch; при MG_EV_MQTT_MSG проходит по списку подписчиков,
 * проверяя соответствие топика их фильтрам.
 */
void tim::p::mqtt_client::handle_events(mg_connection *c, int ev, void *ev_data)
{
    tim::p::mqtt_client *self = (tim::p::mqtt_client *)c->fn_data;
    if (!self)
        return; // mqtt_client::stop() уже отвязал нас от этого соединения.

    switch (ev)
    {
        case MG_EV_OPEN:
            break;

        case MG_EV_CONNECT:
        {
            TIM_TRACE(debug,
                      "TCP connection to MQTT broker '%s' established.",
                      self->_url.c_str());
            if (c->is_tls)
            {
                const std::filesystem::path base_path = tim::application::data_dir() / "tls";
                mg_tls_opts opts =
                {
                    .ca = mg_unpacked((base_path / "ca-cert.pem").string().c_str()),
                    .cert = mg_unpacked((base_path / "cert.pem").string().c_str()),
                    .key = mg_unpacked((base_path / "key.pem").string().c_str())
                };
                mg_tls_init(c, &opts);
            }

#ifdef TIM_DEBUG
//            c->is_hexdumping = 1;
#endif

            break;
        }

        case MG_EV_MQTT_OPEN:
            TIM_TRACE(debug,
                      "MQTT handshake with broker '%s' succeeded.",
                      self->_url.c_str());
            self->_connected = true;
            // Сначала восстанавливаем состояние на брокере (подписки,
            // неподтверждённые публикации), затем уведомляем подсистемы:
            // их обработчики connected должны видеть уже оформленные
            // подписки.
            self->restore_broker_state();
            self->_q->connected();
            break;

        case MG_EV_MQTT_CMD:
        {
            mg_mqtt_message *msg = (mg_mqtt_message *)ev_data;
            switch (msg->cmd)
            {
                case MQTT_CMD_PINGREQ:
                    mg_mqtt_pong(c);
                    break;

                case MQTT_CMD_PUBACK:
                {
                    // Брокер подтвердил публикацию с QoS 1 — исключаем её
                    // из очереди неподтверждённых.
                    const std::uint16_t id = msg->id;
                    const std::vector<outbox_entry>::iterator it = std::find_if(
                        self->_outbox.begin(), self->_outbox.end(),
                        [id](const outbox_entry &e){ return e.packet_id == id; });
                    if (it != self->_outbox.end())
                        self->_outbox.erase(it);
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case MG_EV_MQTT_MSG:
            if (!c->is_draining)
            {
                mg_mqtt_message *msg = (mg_mqtt_message *)ev_data;
                const tim::mqtt_topic topic(std::string(msg->topic.buf, msg->topic.len));

                TIM_TRACE(debug,
                          "MQTT message received at topic '%s': '%.*s'.",
                          topic.c_str(),
                          (int)msg->data.len, msg->data.buf);

                // Снимок id подписчиков, чтобы обработчик мог безопасно (от)подписаться во время диспетчеризации.
                std::vector<std::size_t> ids;
                ids.reserve(self->_subscribers.size());
                for (const subscriber_entry &e: self->_subscribers)
                    ids.push_back(e.id);

                for (std::size_t id: ids)
                {
                    // Подписчик мог быть удалён предыдущим обработчиком;
                    // перепроверяем найдём ли его перед вызовом handler.
                    const subscribers::const_iterator it = std::find_if(
                        self->_subscribers.cbegin(), self->_subscribers.cend(),
                        [id](const subscriber_entry &e){ return e.id == id; });
                    if (it == self->_subscribers.cend())
                        continue;
                    if (tim::topic_matches(topic.view(), it->filter.view()))
                        it->handler(topic, msg->data.buf, msg->data.len);
                }
            }
            break;

        case MG_EV_CLOSE:
        {
            TIM_TRACE(debug,
                      "MQTT connection to broker '%s' closed.",
                      self->_url.c_str());
            self->_client = nullptr;
            self->_connected = false;
            self->_q->disconnected();
            break;
        }

        case MG_EV_ERROR:
            if (!c->is_draining)
            {
                TIM_TRACE(error,
                          TIM_TR("MQTT network error: %s"_en,
                                 "Сетевая ошибка MQTT: %s"_ru),
                          (char *)ev_data);
                c->is_draining = 1;
            }
            self->_connected = false;
            self->_q->disconnected();
            break;
    }
}

/**
 * Mongoose-обработчик таймера. На каждом срабатывании: если соединение
 * активно — отправляет PING, если нет — открывает новое (повторная
 * попытка подключения).
 */
void tim::p::mqtt_client::ping(void *data)
{
    tim::p::mqtt_client *self = (tim::p::mqtt_client *)data;
    assert(self);

    if (self->_client)
    {
        mg_mqtt_ping(self->_client);
        return;
    }

    const mg_mqtt_opts opts =
    {
        .client_id = mg_str(tim::application::name().c_str()),
        .topic = mg_str("client/status"),
        .message = mg_str("disconnected"),
        .qos = TIM_MQTT_QOS,
        .version = 5,
        .keepalive = 0, // Не разрывать соединение.
        .clean = true
    };

    if (!(self->_client = mg_mqtt_connect(self->_mg, self->_url.c_str(), &opts,
                                          &tim::p::mqtt_client::handle_events, self)))
        TIM_TRACE(error,
                  TIM_TR("Failed to connect to MQTT broker at '%s'; will retry."_en,
                         "Ошибка при подключении к брокеру MQTT '%s'; повторим попытку."_ru),
                  self->_url.c_str());
}

/**
 * Восстанавливает состояние клиента на брокере после MQTT-рукопожатия.
 *
 * Клиент подключается с clean-сессией, поэтому брокер не помнит ни
 * подписок, ни неподтверждённых публикаций. Повторно оформляем все
 * зарегистрированные подписки, затем отправляем содержимое outbox:
 * ранее отправленные записи — повторно с прежним packet id (флаг DUP),
 * накопленные без соединения — как новые публикации.
 */
void tim::p::mqtt_client::restore_broker_state()
{
    assert(_client);

    for (const subscriber_entry &e: _subscribers)
    {
        const mg_mqtt_opts sub_opts =
        {
            .topic = mg_str_n(e.filter.data(), e.filter.size()),
            .qos = e.qos
        };
        mg_mqtt_sub(_client, &sub_opts);
    }

    for (outbox_entry &e: _outbox)
    {
        const mg_mqtt_opts pub_opts =
        {
            .topic = mg_str_n(e.topic.data(), e.topic.size()),
            .message = mg_str_n(e.payload.data(), e.payload.size()),
            .qos = e.qos,
            .retransmit_id = e.packet_id, // 0 — новая публикация, иначе DUP.
            .retain = e.retain
        };
        e.packet_id = mg_mqtt_pub(_client, &pub_opts);
    }

    if (!_outbox.empty())
        TIM_TRACE(debug, "Re-sent %zu unacknowledged MQTT publication(s).",
                  _outbox.size());
}
