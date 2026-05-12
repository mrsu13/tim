#include "tim_mqtt_client.h"

#include "tim_mqtt_client_p.h"

#include "tim_application.h"
#include "tim_file_tools.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include "mongoose.h"

#include <algorithm>
#include <cassert>


static const int TIM_MQTT_QOS = 1;

// Открытые

tim::mqtt_client::mqtt_client(mg_mgr *mg)
    : connected{}
    , disconnected{}
    , _d(new tim::p::mqtt_client(this))
{
    assert(mg);
    _d->_mg = mg;
}

tim::mqtt_client::~mqtt_client()
{
    stop();
}

bool tim::mqtt_client::start(std::string_view url, const std::chrono::seconds ping_interval)
{
    assert(!url.empty() && "MQTT broker URL must not be empty.");
    assert(ping_interval.count() && "ping_interval must not be zero.");

    if (_d->_timer)
    {
        TIM_TRACE(Warning, "mqtt_client::start() called while already started; ignoring.");
        return true;
    }

    _d->_url = url;
    _d->_timer = mg_timer_add(_d->_mg, ping_interval.count() * 1000,
                              MG_TIMER_REPEAT | MG_TIMER_RUN_NOW,
                              &tim::p::mqtt_client::ping, _d.get());
    if (!_d->_timer)
    {
        TIM_TRACE(Error,
                  TIM_TR("Failed to start MQTT client timer for broker '%.*s'."_en,
                         "Не могу создать таймер MQTT клиента для брокера '%.*s'."_ru),
                  (int)url.size(), url.data());
        _d->_url.clear();
        return false;
    }
    return true;
}

void tim::mqtt_client::stop()
{
    if (_d->_timer)
    {
        mg_timer_free(&_d->_mg->timers, _d->_timer);
        _d->_timer = nullptr;
    }
    if (_d->_client)
    {
        // Отвязываем обработчики от соединения: mg_mgr_free дёрнет MG_EV_CLOSE
        // на всех живых соединениях, и без этого обработчик попытается
        // прочитать уже освобождённый _d (UAF, видный по мусору в логе
        // вида "broker '<garbage>' closed").
        _d->_client->fn_data = nullptr;
        _d->_client->is_draining = 1;
        _d->_client = nullptr;
    }
    _d->_connected = false;
    _d->_url.clear();
}

bool tim::mqtt_client::is_connected() const
{
    return _d->_connected;
}

void tim::mqtt_client::publish(const tim::mqtt_topic &topic,
                               const char *data, std::size_t size,
                               std::uint8_t qos,
                               bool retain)
{
    assert(!topic.empty() && "Topic must not be empty.");

    const mg_mqtt_opts pub_opts =
    {
        .topic = mg_str_n(topic.data(), topic.size()),
        .message = mg_str_n(data, size),
        .qos = qos,
        .retain = retain
    };

    mg_mqtt_pub(_d->_client, &pub_opts);

    TIM_TRACE(Debug, "Published to '%s': '%.*s'.",
              topic.c_str(),
              (int)size, data);
}

void tim::mqtt_client::publish(const tim::mqtt_topic &topic,
                               std::string_view payload,
                               std::uint8_t qos,
                               bool retain)
{
    publish(topic, payload.data(), payload.size(), qos, retain);
}

tim::mqtt_subscription tim::mqtt_client::subscribe(const tim::mqtt_topic &topic_filter,
                                                   message_handler mh,
                                                   std::uint8_t qos)
{
    assert(!topic_filter.empty() && "Topic filter must not be empty.");
    assert(mh);

    const std::size_t id = _d->_next_subscriber_id++;
    _d->_subscribers.push_back({ id, topic_filter, std::move(mh) });

    const mg_mqtt_opts sub_opts =
    {
        .topic = mg_str_n(topic_filter.data(), topic_filter.size()),
        .qos = qos
    };

    mg_mqtt_sub(_d->_client, &sub_opts);

    TIM_TRACE(Debug, "Subscribed to '%s'.", topic_filter.c_str());

    return tim::mqtt_subscription(this, id);
}

void tim::mqtt_client::unsubscribe(std::size_t id)
{
    const tim::p::mqtt_client::subscribers::iterator it = std::find_if(
        _d->_subscribers.begin(), _d->_subscribers.end(),
        [id](const tim::p::mqtt_client::subscriber_entry &e){ return e.id == id; });
    if (it == _d->_subscribers.end())
        return;

    TIM_TRACE(Debug, "Unsubscribed from '%s'.", it->filter.c_str());
    _d->_subscribers.erase(it);
}


// Закрытые

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
            TIM_TRACE(Debug,
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
            TIM_TRACE(Debug,
                      "MQTT handshake with broker '%s' succeeded.",
                      self->_url.c_str());
            self->_connected = true;
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

                TIM_TRACE(Debug,
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
            TIM_TRACE(Debug,
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
                TIM_TRACE(Error,
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
        .keepalive = 0, // Do not disconnect.
        .clean = true
    };

    if (!(self->_client = mg_mqtt_connect(self->_mg, self->_url.c_str(), &opts,
                                          &tim::p::mqtt_client::handle_events, self)))
        TIM_TRACE(Error,
                  TIM_TR("Failed to connect to MQTT broker at '%s'; will retry."_en,
                         "Ошибка при подключении к брокеру MQTT '%s'; повторим попытку."_ru),
                  self->_url.c_str());
}
