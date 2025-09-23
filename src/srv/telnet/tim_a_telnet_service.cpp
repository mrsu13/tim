#include "tim_a_telnet_service.h"

#include "tim_a_telnet_service_p.h"

#include "tim_a_inetd_service.h"
#include "tim_string_tools.h"
#include "tim_trace.h"

#include "libtelnet.h"
#include "mongoose.h"

#include <cassert>


// Public

tim::a_telnet_service::~a_telnet_service()
{
    telnet_free(_d->_telnet);
}

/**
 * \bug
 */
std::size_t tim::a_telnet_service::cols() const
{
    return 80;
}

/**
 * \bug
 */
void tim::a_telnet_service::clear()
{
}

int tim::a_telnet_service::vprintf(const char *format, va_list args)
{
    assert(format && *format);

    std::string s;

    va_list args_copy;
    va_copy(args_copy, args);
    const int n = tim::vsprintf(s, format, args);
    va_end(args_copy);

    if (n > 0)
        write(s.c_str(), n);

    return n;
}

int tim::a_telnet_service::printf(const char *format, ... )
{
    assert(format && *format);

    va_list args;
    va_start(args, format);
    const int n = tim::a_telnet_service::vprintf(format, args);
    va_end(args);

    return n;
}

// Protected

tim::a_telnet_service::a_telnet_service(const std::string &name, mg_connection *c)
    : tim::a_inetd_service(name, c)
    , _d(new tim::p::a_telnet_service(this))
{
    _d->_telnet = telnet_init(nullptr,
                              &tim::p::a_telnet_service::event_handler,
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


// Private

bool tim::a_telnet_service::ready_read(const char *data, std::size_t size, std::size_t *bytes_read)
{
    telnet_recv(_d->_telnet, data, size);
    if (bytes_read)
        *bytes_read = size;
    return true;
}

bool tim::a_telnet_service::ready_write(const char *data, std::size_t size, std::size_t *bytes_written)
{
    assert(data);

    telnet_send_text(_d->_telnet, data, size);
    if (bytes_written)
        *bytes_written = size;

    return true;
}

void tim::p::a_telnet_service::event_handler(telnet_t *telnet, telnet_event_t *event, void *data)
{
    (void) telnet;

    tim::p::a_telnet_service *self = (tim::p::a_telnet_service *)data;
    assert(self);

    switch (event->type)
    {
        case TELNET_EV_DATA:
            if (!self->_q->process_data(event->data.buffer, event->data.size))
                self->_q->close();
            break;

        case TELNET_EV_SEND:
            mg_send(self->_q->connection(), event->data.buffer, event->data.size);
            break;

        case TELNET_EV_ERROR:
            TIM_TRACE(Error, "Telnet error: %s", event->error.msg);
            break;

        default:
            break;
    }
}
