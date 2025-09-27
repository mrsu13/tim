#include "tim_prompt_service.h"

#include "tim_prompt_service_p.h"

#include "tim_config.h"
#include "tim_string_tools.h"
#include "tim_tcl.h"
#include "tim_telnet.h"
#include "tim_vt.h"
#include "tim_vt_shell.h"

#include "fort.h"


static const std::string LOREM_IPSUM =
"Lorem ipsum dolor sit amet, consectetur adipiscing elit, \
sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. \
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris \
nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in \
reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla \
pariatur. Excepteur sint occaecat cupidatat non proident, \
sunt in culpa qui officia deserunt mollit anim id est laborum.";

// Public

tim::prompt_service::prompt_service(mg_connection *c)
    : tim::a_inetd_service("prompt", c)
    , _d(new tim::p::prompt_service(this))
{
    _d->_terminal.reset(new tim::vt(this));
    _d->_telnet.reset(new tim::telnet(this));
    _d->_tcl.reset(new tim::tcl(_d->_terminal.get()));
    _d->_shell.reset(new tim::vt_shell(_d->_terminal.get(), _d->_tcl.get()));

    _d->_telnet->ready_read.connect(
        std::bind(&tim::p::prompt_service::on_ready_read, _d.get(),
                  std::placeholders::_1, std::placeholders::_2));
}

tim::prompt_service::~prompt_service() = default;


// Private

void tim::p::prompt_service::on_ready_read(const char *data, std::size_t size)
{
    assert(data);

    if (!size)
        return;

    if (size < 2
            || data[0] != tim::COMMAND_PREFIX)
        cloud(std::strncmp(data, "lorem", size) == 0
                    ? LOREM_IPSUM
                    : std::string(data, size));
    else
        _shell->eval(data + 1, size - 1);
}

void tim::p::prompt_service::cloud(const std::string &text, const tim::color &bg_color)
{
    _shell->terminal()->set_bg_color(bg_color);
    _shell->terminal()->set_color(bg_color.text_color());

    ft_table_t *table = ft_create_table();
    ft_u8write_ln(table,
                  tim::aligned(text, tim::text_align::Justify,
                               text.size() > _telnet->cols() * 2
                                   ? _telnet->cols() / 2
                                   : _telnet->cols() / 3).c_str());
    const std::string_view table_text((const char *)ft_to_u8string(table));
    _telnet->write(table_text.data(), table_text.size() - 1); // No \n at the end.
    ft_destroy_table(table);

    _shell->terminal()->reset_colors();
}
