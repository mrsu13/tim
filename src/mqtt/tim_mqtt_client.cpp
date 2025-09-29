#include "tim_mqtt_client.h"

#include "tim_mqtt_client_p.h"

#include "tim_application.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include "mongoose.h"

#include <cassert>


static const int TIM_MQTT_QOS = 1;

// Public

tim::mqtt_client::mqtt_client(mg_mgr *mg, std::uint16_t port, const std::string &host)
    : _d(new tim::p::mqtt_client(this))
{
    assert(mg);
    assert(port > 1024 && "Port number must be greater than 1024.");
    assert(!host.empty() && "MQTT broker host address must not be empty.");

    _d->_host = host;
    _d->_port = port;

    {
        const mg_mqtt_opts opts =
        {
            .client_id = mg_str(tim::application::name().c_str()),
            .qos = TIM_MQTT_QOS,
            .version = 5,
            .keepalive = 5,
            .clean = true
        };

        char url[512];
        std::snprintf(url, sizeof(url), "mqtt://%s:%u", _d->_host.c_str(), _d->_port);
        if (!(_d->_client = mg_mqtt_connect(mg, url, &opts, tim::p::mqtt_client::handle_events, _d.get())))
            TIM_TRACE(Fatal,
                      TIM_TR("Failed to connect to MQTT broker at '%s'."_en,
                             "Ошибка при подключении к брокеру MQTT '%s'."_ru),
                      url);
    }
}

tim::mqtt_client::~mqtt_client() = default;

void tim::mqtt_client::publish(const std::string &topic,
                               const char *data, std::size_t size,
                               std::uint8_t qos,
                               bool retain)
{
    assert(!topic.empty() && "Topic must not be empty.");

    const mg_mqtt_opts pub_opts =
    {
        .topic = mg_str(topic.c_str()),
        .message = mg_str_n(data, size),
        .qos = qos,
        .retain = retain
    };

    mg_mqtt_pub(_d->_client, &pub_opts);
}

void tim::mqtt_client::subscribe(const std::string &topic, message_handler mh, std::uint8_t qos)
{
    assert(!topic.empty() && "Topic must not be empty.");
    assert(mh);

    _d->_subscribers.emplace(topic, mh);

    const mg_mqtt_opts pub_opts =
    {
        .topic = mg_str(topic.c_str()),
        .qos = qos
    };

    mg_mqtt_sub(_d->_client, &pub_opts);
}


// Private

void tim::p::mqtt_client::handle_events(mg_connection *c, int ev, void *ev_data)
{
    tim::p::mqtt_client *self = (tim::p::mqtt_client *)c->fn_data;
    assert(self);

    switch (ev)
    {
        case MG_EV_OPEN:
            break;

        case MG_EV_CONNECT:
        {
            TIM_TRACE(Debug,
                      "TCP connection to MQTT broker '%s:%u' established.",
                      self->_host.c_str(), self->_port);
            /* For the future support of TLS.
            if (mg_url_is_ssl(s_lsn))
            {
                struct mg_tls_opts opts =
                {
                    .ca = mg_unpacked("/certs/ss_ca.pem"),
                    .cert = mg_unpacked("/certs/ss_server.pem"),
                    .key = mg_unpacked("/certs/ss_server.pem")
                };
                mg_tls_init(c, &opts);
            } */

#ifdef TIM_DEBUG
            c->is_hexdumping = 1;
#endif

            break;
        }

        case MG_EV_MQTT_OPEN:
            TIM_TRACE(Debug,
                      "MQTT handshake with broker '%s:%u' succeeded.",
                      self->_host.c_str(), self->_port);
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
                const std::string topic(msg->topic.buf, msg->topic.len);

                TIM_TRACE(Debug,
                          "MQTT message received at topic '%s': '%.*s'.",
                          topic.c_str(),
                          (int)msg->data.len, msg->data.buf);

                for (auto[i, e] = self->_subscribers.equal_range(topic); i != e; ++i)
                    i->second(topic, msg->data.buf, msg->data.len);
            }
            break;

        case MG_EV_CLOSE:
        {
            TIM_TRACE(Debug,
                      "MQTT connection to broker '%s:%u' closed.",
                      self->_host.c_str(), self->_port);
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
            break;
    }
}
