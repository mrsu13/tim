#include "tim_tcl_shell.h"

#include "tim_telnet_service.h"
#include "tim_telnet_service_p.h"

#include "tim_config.h"
#include "tim_inetd_service.h"
#include "tim_line_edit.h"
#include "tim_trace.h"

#include "mongoose.h"
#include "tcl.h"

#include <assert.h>
#include <stdlib.h>


typedef struct tim_tcl_shell
{
    tim_telnet_service_t super;
    struct tcl tcl;
    tim_line_edit_t *ledit;
} tim_tcl_shell_t;

static const char *tim_tcl_shell_welcome_banner();
static const char *tim_tcl_shell_bye_banner();
static const char *tim_tcl_shell_prompt(const tim_tcl_shell_t *srv);
static bool tim_tcl_shell_process_data(void *srv, const char *data, size_t size);
static int tim_tcl_shell_write_data(void *srv, const char *data, size_t size);

tim_tcl_shell_t *tim_tcl_shell_new(struct mg_connection *c)
{
    tim_tcl_shell_t *srv = (tim_tcl_shell_t *)calloc(1, sizeof(tim_tcl_shell_t));
    assert(srv);

    tim_telnet_service_init(&srv->super, c);
    srv->super.process_data = &tim_tcl_shell_process_data;
    tcl_init(&srv->tcl);

    srv->ledit = tim_line_edit_new(tim_tcl_shell_write_data, srv);
    tim_inetd_service_write_str((tim_inetd_service_t *)srv, tim_tcl_shell_welcome_banner());

    tim_line_edit_new_line(srv->ledit, tim_tcl_shell_prompt(srv));

    return srv;
}

void tim_tcl_shell_free(tim_tcl_shell_t *srv)
{
    assert(srv);
    tim_inetd_service_write_str((tim_inetd_service_t *)srv, tim_tcl_shell_bye_banner());
    tcl_destroy(&srv->tcl);
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

const char *tim_tcl_shell_prompt(const tim_tcl_shell_t *srv)
{
    (void) srv;
    return "> ";
}

bool tim_tcl_shell_process_data(void *srv, const char *data, size_t size)
{
    tim_tcl_shell_t *self = (tim_tcl_shell_t *)srv;
    assert(self);
    assert(data);

    switch (tim_line_edit_get_line(self->ledit, data, size))
    {
        case TimLineEditFinished:
        {
            char *line = tim_line_edit_line(self->ledit);
            tim_tcl_shell_write_data(self, "\n", 1);
            TIM_TRACE(Debug, "Command to process: '%s'", line);
            if (tcl_eval(&self->tcl, line, strlen(line)) != FERROR)
                tim_tcl_shell_write_data(self, tcl_string(self->tcl.result), tcl_length(self->tcl.result));
            else
            {
                static const char ERROR_MSG[] = "TCL error.";
                tim_tcl_shell_write_data(self, ERROR_MSG, sizeof(ERROR_MSG) - 1);
            }
            free(line);
            tim_line_edit_new_line(self->ledit, tim_tcl_shell_prompt(self));
            break;
        }

        case TimLineEditContinue:
            break;

        case TimLineEditExit:
            tim_inetd_service_write_str((tim_inetd_service_t *)self, tim_tcl_shell_bye_banner());
            return false;

        case TimLineEditError:
            return false;
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
