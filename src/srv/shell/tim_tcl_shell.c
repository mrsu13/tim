#include "tim_tcl_shell.h"

#include "tim_telnet_service.h"
#include "tim_telnet_service_p.h"

#include "tim_config.h"
#include "tim_inetd_service.h"
#include "tim_line_edit.h"
#include "tim_tcl.h"
#include "tim_trace.h"

#include "lil.h"

#include <assert.h>
#include <stdlib.h>


typedef struct tim_tcl_shell
{
    tim_telnet_service_t super;
    tim_tcl_t *tcl;
    tim_line_edit_t *ledit;
    bool new_line;
} tim_tcl_shell_t;

static const char *tim_tcl_shell_welcome_banner();
static const char *tim_tcl_shell_bye_banner();
static bool tim_tcl_shell_process_data(void *srv, const char *data, size_t size);
static int tim_tcl_shell_write_data(void *srv, const char *data, size_t size);
static void tim_tcl_shell_tcl_write(lil_t lil, const char *msg);

tim_tcl_shell_t *tim_tcl_shell_new(struct mg_connection *c)
{
    tim_tcl_shell_t *srv = (tim_tcl_shell_t *)calloc(1, sizeof(tim_tcl_shell_t));
    assert(srv);

    tim_telnet_service_init(&srv->super, c);
    srv->super.process_data = &tim_tcl_shell_process_data;

    srv->tcl = tim_tcl_new(&tim_tcl_shell_tcl_write, NULL);
    assert(srv->tcl);

    srv->ledit = tim_line_edit_new(tim_tcl_shell_write_data, srv);
    assert(srv->ledit);
    tim_inetd_service_write_str((tim_inetd_service_t *)srv, tim_tcl_shell_welcome_banner());

    tim_line_edit_new_line(srv->ledit, tim_tcl_prompt(srv->tcl));

    return srv;
}

void tim_tcl_shell_free(tim_tcl_shell_t *srv)
{
    assert(srv);
    tim_inetd_service_write_str((tim_inetd_service_t *)srv, tim_tcl_shell_bye_banner());
    tim_line_edit_free(srv->ledit);
    tim_tcl_free(srv->tcl);
    tim_telnet_service_destroy(&srv->super);
    free(srv);
}


// Static

const char *tim_tcl_shell_welcome_banner()
{
    return "Welcome to TIM!";
}

const char *tim_tcl_shell_bye_banner()
{
    return "Bye!\n";
}

bool tim_tcl_shell_process_data(void *srv, const char *data, size_t size)
{
    tim_tcl_shell_t *self = (tim_tcl_shell_t *)srv;
    assert(self);
    assert(data);

    if (!tim_tcl_evaluating(self->tcl))
    {
        if (self->new_line)
        {
            self->new_line = false;
            tim_line_edit_new_line(self->ledit, tim_tcl_prompt(self->tcl));
        }

        switch (tim_line_edit_get_line(self->ledit, data, size))
        {
            case TimLineEditFinished:
            {
                self->new_line = true;
                if (!tim_line_edit_empty(self->ledit))
                {
                    tim_inetd_service_write_str((tim_inetd_service_t *)self, "\n");
                    char *line = tim_line_edit_line(self->ledit);
                    // _d->_ledit->history_save(_d->_history_path);

                    const char *res;
                    if (tim_tcl_eval(self->tcl, line, &res, self))
                    {
                        if (res
                                && *res)
                            tim_inetd_service_write_str((tim_inetd_service_t *)self, res);
                    }
                    else
                    {
                        const size_t pos = tim_tcl_error_pos(self->tcl);

                        // set_color(theme().colors.at(t2::vt_color_index::Error));
                        tim_inetd_service_write_str((tim_inetd_service_t *)self, "Error: ");
                        tim_inetd_service_write_str((tim_inetd_service_t *)self, tim_tcl_error_msg(self->tcl));
                        tim_inetd_service_write_str((tim_inetd_service_t *)self, "\n");
                        tim_inetd_service_write_str((tim_inetd_service_t *)self, line);
                        tim_inetd_service_write_str((tim_inetd_service_t *)self, "\n");
                        if (pos)
                        {
                            static const char hr[] = "─";
                            for (size_t i = 0; i < pos - 1; ++i)
                                tim_inetd_service_write((tim_inetd_service_t *)self, hr, sizeof(hr) - 1, NULL);
                        }
                        {
                            static const char arrow[] = "┘";
                            tim_inetd_service_write((tim_inetd_service_t *)self, arrow, sizeof(arrow) - 1, NULL);
                        }
                        // reset_colors();
                    }
                    free(line);
                }
                break;
            }
            case TimLineEditContinue:
                break;

            case TimLineEditExit:
                tim_inetd_service_write_str((tim_inetd_service_t *)self, tim_tcl_shell_bye_banner());
                return false;

            case TimLineEditError:
                self->new_line = true;
                break;
        }
    }

    return true;
}

int tim_tcl_shell_write_data(void *srv, const char *data, size_t size)
{
    tim_tcl_shell_t *self = (tim_tcl_shell_t *)srv;
    assert(self);
    assert(data);

    return tim_inetd_service_write((tim_inetd_service_t *)self, data, size, NULL)
                ? size
                : -1;
}

void tim_tcl_shell_tcl_write(lil_t lil, const char *msg)
{
    assert(lil);
    assert(msg);

    tim_tcl_shell_t *self = (tim_tcl_shell_t *)lil_get_data(lil);
    assert(self);

    tim_inetd_service_write_str((tim_inetd_service_t *)self, msg);
}
