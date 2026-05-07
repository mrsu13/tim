#include "tim_prompt_service.h"

#include "tim_prompt_service_p.h"

#include "tim_mqtt_client.h"
#include "tim_prompt_shell.h"
#include "tim_string_tools.h"
#include "tim_tcl.h"
#include "tim_telnet_server.h"
#include "tim_trace.h"
#include "tim_vt.h"


// Public

tim::prompt_service::prompt_service(mg_connection *c, tim::mqtt_client &mqtt)
    : tim::a_inetd_service("prompt", c)
    , _d(new tim::p::prompt_service(this, mqtt))
{
    _d->_telnet.reset(new tim::telnet_server(this));
    _d->_terminal.reset(new tim::vt(_d->_telnet.get()));
    _d->_tcl.reset(new tim::tcl(_d->_terminal.get(), _d->_user.id, mqtt));
    _d->_shell.reset(new tim::prompt_shell(_d->_terminal.get(), _d->_tcl.get()));

    _d->_topic = tim::mqtt_topic("post") / std::to_string(id());

    _d->_on_data_ready = _d->_telnet->data_ready.connect(
        [d = _d.get()](const char *data, std::size_t size){ d->on_data_ready(data, size); });

    _d->_on_posted = _d->_shell->posted.connect(
        [d = _d.get()](const std::string &text)
        {
            if (d->_mqtt.is_connected())
                d->_mqtt.publish(d->_topic, text.c_str(), text.size());
        });

    _d->_on_connected = mqtt.connected.connect(
        [d = _d.get()]{ d->subscribe(); });

    if (mqtt.is_connected())
        _d->subscribe();
}

tim::prompt_service::~prompt_service() = default;


// Private

void tim::p::prompt_service::subscribe()
{
    _mqtt.publish("user/connect", _user.id.to_string());
    _sub_post = _mqtt.subscribe(_topic.parent() / "+",
        [this](const tim::mqtt_topic &topic, const char *data, std::size_t size)
        { on_post(topic, data, size); });
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
    if (topic != _topic)
    {
        _shell->cloud(_user.title(),
                      '\n' + std::string(data, size),
                      _shell->terminal()->color(
                        tim::to_int(std::string(topic.last_level())) % (_shell->terminal()->color_count() - 1) + 1));
        _shell->new_line();
    }
}
