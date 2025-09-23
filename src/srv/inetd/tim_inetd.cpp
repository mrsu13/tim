#include "tim_inetd.h"

#include "tim_inetd_p.h"

#include "tim_a_inetd_service.h"
#include "tim_trace.h"

#include "mongoose.h"

#include <cassert>


tim::inetd::~inetd() = default;


// Private

tim::inetd::inetd(mg_mgr *mg, std::uint16_t port, service_factory factory)
    : tim::service("inetd")
    , _d(new tim::p::inetd())
{
    assert(mg);
    assert(port > 1024 && "Port number must be greater than 1024.");
    assert(factory);

    _d->_port = port;
    _d->_factory = factory;

    {
        char url[128];
        std::snprintf(url, sizeof(url), "tcp://0.0.0.0:%u", _d->_port);
        if (!(_d->_server = mg_listen(mg, url, tim::p::inetd::handle_events, _d.get())))
            TIM_TRACE(Fatal, "Failed to create inetd at port %u.", _d->_port);
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
                TIM_TRACE(Debug, "inetd is listening at port %u.", self->_port);
            break;

        case MG_EV_ACCEPT:
        {
            TIM_TRACE(Debug, "inetd accepted a connection at port %u.", self->_port);
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

            std::unique_ptr<tim::a_inetd_service> srv = self->_factory(c);
            if (srv)
                self->_connections.emplace(c, std::move(srv));
            else
            {
                TIM_TRACE(Error, "Failed to create inetd service at port %u.", self->_port);
                c->is_draining = 1;
            }
            break;
        }

        case MG_EV_READ:
            if (!c->is_draining)
            {
                connection_map::const_iterator f = self->_connections.find(c);
                assert(f != self->_connections.cend());
                if (!f->second->read())
                    f->second->close();
            }
            break;

        case MG_EV_CLOSE:
        {
            TIM_TRACE(Debug, "inetd connection at port %u disconnected.", self->_port);
            connection_map::iterator f = self->_connections.find(c);
            assert(f != self->_connections.end());
            self->_connections.erase(f);
            break;
        }

        case MG_EV_ERROR:
            if (!c->is_draining)
            {
                TIM_TRACE(Error, "Error: %s", (char *)ev_data);
                connection_map::iterator f = self->_connections.find(c);
                assert(f != self->_connections.end());
                self->_connections.erase(f);
            }
            break;
    }
}
