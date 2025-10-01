#include "tim_inetd.h"

#include "tim_inetd_p.h"

#include "tim_a_inetd_service.h"
#include "tim_file_tools.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include "mongoose.h"

#include <cassert>


// Public

tim::inetd::~inetd() = default;


// Private

tim::inetd::inetd(mg_mgr *mg,
                  std::uint16_t port,
                  bool tls_enabled,
                  const std::string &if_addr,
                  service_factory factory)
    : tim::service("inetd")
    , _d(new tim::p::inetd())
{
    assert(mg);
    assert(port && "port must not be positive.");
    assert(factory);

    _d->_if_addr = if_addr.empty()
                        ? "0.0.0.0"
                        : if_addr;
    _d->_port = port;
    _d->_tls_enabled = tls_enabled;
    _d->_factory = factory;

    {
        char url[128];
        std::snprintf(url, sizeof(url), "tcp://%s:%u", _d->_if_addr.c_str(), _d->_port);
        if (!(_d->_server = mg_listen(mg, url, tim::p::inetd::handle_events, _d.get())))
            TIM_TRACE(Fatal,
                      TIM_TR("Failed to instantiate inetd at '%s:%u'."_en,
                             "Ошибка при попытке создать экземпляр inetd на '%s:%u'."_ru),
                      _d->_if_addr.c_str(), _d->_port);
    }
}

void tim::p::inetd::handle_events(mg_connection *c, int ev, void *ev_data)
{
    tim::p::inetd *self = (tim::p::inetd *)c->fn_data;
    assert(self);

    switch (ev)
    {
        case MG_EV_OPEN:
            if (c->is_listening == 1)
                TIM_TRACE(Debug, "inetd is listening at '%s:%u'.",
                          self->_if_addr.c_str(), self->_port);
            break;

        case MG_EV_ACCEPT:
        {
            TIM_TRACE(Debug, "inetd accepted a connection at '%s:%u'.",
                      self->_if_addr.c_str(), self->_port);
            if (self->_tls_enabled)
            {
                const std::filesystem::path base_path = tim::standard_location(tim::filesystem_location::AppTlsData);
                mg_tls_opts opts =
                {
                    .ca = mg_unpacked((base_path / "ca.crt").string().c_str()),
                    .cert = mg_unpacked((base_path / "client.crt").string().c_str()),
                    .key = mg_unpacked((base_path / "client.key").string().c_str())
                };
                mg_tls_init(c, &opts);
            }

#ifndef TIM_DEBUG
//            c->is_hexdumping = 1;
#endif

            std::unique_ptr<tim::a_inetd_service> srv = self->_factory(c);
            if (srv)
                self->_connections.emplace(c, std::move(srv));
            else
            {
                TIM_TRACE(Error,
                          TIM_TR("Failed to instantiate inetd service at '%s:%u'."_en,
                                 "Ошибка при попытке запустить сервис inetd на '%s:%u'."_ru),
                          self->_if_addr.c_str(), self->_port);
                c->is_draining = 1;
            }
            break;
        }

        case MG_EV_READ:
            if (!c->is_draining)
            {
                connection_map::const_iterator f = self->_connections.find(c);
                assert(f != self->_connections.cend());
                f->second->ready_read();
            }
            break;

        case MG_EV_CLOSE:
        {
            if (c != self->_server)
            {
                TIM_TRACE(Debug, "inetd connection at '%s:%u' closed.",
                          self->_if_addr.c_str(), self->_port);
                connection_map::iterator f = self->_connections.find(c);
                if (f != self->_connections.end())
                    self->_connections.erase(f);
            }
            else
            {
                TIM_TRACE(Debug, "inetd server at '%s:%u' closed.",
                          self->_if_addr.c_str(), self->_port);
                self->_server = nullptr;
            }
            break;
        }

        case MG_EV_ERROR:
            if (!c->is_draining)
            {
                TIM_TRACE(Error,
                          TIM_TR("Network error: %s"_en,
                                 "Сетевая ошибка: %s"_ru),
                          (char *)ev_data);
                connection_map::iterator f = self->_connections.find(c);
                assert(f != self->_connections.end());
                self->_connections.erase(f);
            }
            break;
    }
}
