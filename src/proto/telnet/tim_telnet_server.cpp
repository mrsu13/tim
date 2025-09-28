#include "tim_telnet_server.h"

#include "tim_telnet_server_p.h"

#include "tim_a_io_device.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include "libtelnet.h"

#include <cassert>


// Public

tim::telnet_server::telnet_server(tim::a_io_device *io)
    : tim::a_terminal_protocol(io)
    , _d(new tim::p::telnet_server(this))
{
    _d->_telnet = telnet_init(nullptr,
                              &tim::p::telnet_server::event_handler,
                              TELNET_FLAG_NVT_EOL, // Receive data with translation of the TELNET NVT CR NUL
                                                   // and CR LF sequences specified in RFC854 to C carriage
                                                   // return (\r) and C newline (\n), respectively.
                              _d.get());
    assert(_d->_telnet);

    telnet_negotiate(_d->_telnet, TELNET_DO, TELNET_TELOPT_NAWS);
    telnet_negotiate(_d->_telnet, TELNET_DO, TELNET_TELOPT_TTYPE);
    telnet_negotiate(_d->_telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
    telnet_negotiate(_d->_telnet, TELNET_WILL, TELNET_TELOPT_SGA);
}

tim::telnet_server::~telnet_server()
{
    telnet_free(_d->_telnet);
}

const std::string &tim::telnet_server::terminal_name() const
{
    return _d->_term_name;
}

std::size_t tim::telnet_server::rows() const
{
    return _d->_rows;
}

std::size_t tim::telnet_server::cols() const
{
    return _d->_cols;
}

bool tim::telnet_server::write(const char *data, std::size_t size)
{
    assert(data);

    telnet_send_text(_d->_telnet, data, size);
    return true;
}

void tim::telnet_server::process_raw_data(const char *data, std::size_t size)
{
    telnet_recv(_d->_telnet, data, size);
}


// Private

void tim::p::telnet_server::event_handler(telnet_t *telnet, telnet_event_t *event, void *data)
{
    (void) telnet;

    tim::p::telnet_server *self = (tim::p::telnet_server *)data;
    assert(self);

    switch (event->type)
    {
        case TELNET_EV_DATA:
            self->_q->data_ready(event->data.buffer, event->data.size);
            break;

        case TELNET_EV_SEND:
            self->_q->io()->write(event->data.buffer, event->data.size);
            break;

        case TELNET_EV_SUBNEGOTIATION:
        {
            switch (event->sub.telopt)
            {
                case TELNET_TELOPT_NAWS:
                    if (event->sub.size >= 4)
                    {
                        self->_cols = ((std::uint8_t)event->sub.buffer[0] << 8) | (std::uint8_t)event->sub.buffer[1];
                        self->_rows = ((std::uint8_t)event->sub.buffer[2] << 8) | (std::uint8_t)event->sub.buffer[3];
                        TIM_TRACE(Debug, "Terminal size: %ux%u.", self->_cols, self->_rows);
                    }
                    break;

                case TELNET_TELOPT_TTYPE:
                    self->_term_name = std::string(event->sub.buffer, event->sub.size);
                    TIM_TRACE(Debug, "Terminal name: '%s'.", self->_term_name.c_str());
                    break;

                default:
                    break;
            }
            break;
        }

        case TELNET_EV_ERROR:
            TIM_TRACE(Error,
                      TIM_TR("TELNET error: %s"_en,
                             "Ошибка TELNET: %s"_ru),
                      event->error.msg);
            break;

        default:
            break;
    }
}
