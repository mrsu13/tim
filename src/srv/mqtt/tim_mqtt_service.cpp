#include "tim_mqtt_service.h"

#include "tim_mqtt_service_p.h"

#include "tim_trace.h"
#include "tim_translator.h"

#include "mongoose.h"

#include <cassert>


static const int TIM_MQTT_QOS = 1;

// Public

tim::mqtt_service::mqtt_service(mg_mgr *mg, std::uint16_t port, const std::string &host)
    : tim::service("mqtt")
    , _d(new tim::p::mqtt_service())
{
    assert(mg);
    assert(port > 1024 && "Port number must be greater than 1024.");
    assert(!host.empty() && "MQTT broker host address must not be empty.");

    _d->_host = host;
    _d->_port = port;

    {
        const mg_mqtt_opts opts =
        {
            .message = mg_str("bye"),
            .qos = TIM_MQTT_QOS,
            .version = 4,
            .keepalive = 5,
            .clean = true
        };

        char url[512];
        std::snprintf(url, sizeof(url), "mqtt://%s:%u", _d->_host.c_str(), _d->_port);
        if (!(_d->_client = mg_mqtt_connect(mg, url, &opts, tim::p::mqtt_service::handle_events, _d.get())))
            TIM_TRACE(Fatal,
                      TIM_TR("Failed to connect to MQTT broker at '%s'."_en,
                             "Ошибка при подключении к брокеру MQTT '%s'."_ru),
                      url);
    }
}

tim::mqtt_service::~mqtt_service() = default;


// Private

void tim::p::mqtt_service::handle_events(mg_connection *c, int ev, void *ev_data)
{
    tim::p::mqtt_service *self = (tim::p::mqtt_service *)c->fn_data;
    assert(self);

    switch (ev)
    {
        case MG_EV_OPEN:
            break;

        case MG_EV_CONNECT:
        {
            TIM_TRACE(Debug,
                      "Connected to MQTT broker at '%s:%u'.",
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

#ifndef NDEBUG
//            c->is_hexdumping = 1;
#endif

            break;
        }

        case MG_EV_MQTT_OPEN:
            TIM_TRACE(Debug,
                      "MQTT handshake to broker at '%s:%u' succeeded.",
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
                TIM_TRACE(Debug,
                          "MQTT message received at topic '%.*s': '%.*s'.",
                          (int)msg->topic.len, msg->topic.buf,
                          (int)msg->data.len, msg->data.buf);
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
                TIM_TRACE(Error,
                          TIM_TR("MQTT network error: %s"_en,
                                 "Сетевая ошибка MQTT: %s"_ru),
                          (char *)ev_data);
            break;
    }
}
