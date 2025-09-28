#include "tim_prompt_service.h"

#include "tim_prompt_service_p.h"

#include "tim_prompt_shell.h"
#include "tim_tcl.h"
#include "tim_telnet_server.h"
#include "tim_vt.h"


// Public

tim::prompt_service::prompt_service(mg_connection *c)
    : tim::a_inetd_service("prompt", c)
    , _d(new tim::p::prompt_service(this))
{
    _d->_telnet.reset(new tim::telnet_server(this));
    _d->_terminal.reset(new tim::vt(_d->_telnet.get()));
    _d->_tcl.reset(new tim::tcl(_d->_terminal.get()));
    _d->_shell.reset(new tim::prompt_shell(_d->_terminal.get(), _d->_tcl.get()));

    _d->_telnet->data_ready.connect(
        std::bind(&tim::p::prompt_service::on_data_ready, _d.get(),
                  std::placeholders::_1, std::placeholders::_2));
}

tim::prompt_service::~prompt_service() = default;


// Private

void tim::p::prompt_service::on_data_ready(const char *data, std::size_t size)
{
    assert(data);

    if (size
            && !_shell->write(data, size))
        _q->close();
}
