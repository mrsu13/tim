#include "tim_tcl_shell.h"

#include "tim_telnet_service.h"
#include "tim_telnet_service_p.h"

#include "tim_config.h"
#include "tim_inetd_service.h"
#include "tim_trace.h"

#include "linenoise.h"
#include "mongoose.h"
#include "tcl.h"

#include <assert.h>
#include <stdlib.h>


typedef struct tim_tcl_shell
{
    tim_telnet_service_t super;
    struct tcl tcl;
    struct linenoiseState ls;
    char line_buffer[TIM_LINE_SIZE];
    bool new_line;
} tim_tcl_shell_t;

static const char *tim_tcl_shell_welcome_banner();
static const char *tim_tcl_shell_bye_banner();
static const char *tim_tcl_shell_prompt(const tim_tcl_shell_t *srv);
static bool tim_tcl_shell_process_data(void *srv, const char *data, size_t size);

tim_tcl_shell_t *tim_tcl_shell_new(struct mg_connection *c)
{
    tim_tcl_shell_t *srv = (tim_tcl_shell_t *)calloc(1, sizeof(tim_tcl_shell_t));
    assert(srv);

    tim_telnet_service_init(&srv->super, c);
    srv->super.process_data = &tim_tcl_shell_process_data;
    tcl_init(&srv->tcl);

    tim_inetd_service_write_str((tim_inetd_service_t *)srv, tim_tcl_shell_welcome_banner());
    tim_inetd_service_write_str((tim_inetd_service_t *)srv, tim_tcl_shell_prompt(srv));

    srv->new_line = true;

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
    return "";
}

const char *tim_tcl_shell_bye_banner()
{
    return "";
}

const char *tim_tcl_shell_prompt(const tim_tcl_shell_t *srv)
{
    (void) srv;
    return "> ";
}

bool tim_tcl_shell_process_data(void *srv, const char *data, size_t size)
{
    tim_tcl_shell_t *self = (tim_tcl_shell_t *)srv;
    assert(srv);
    assert(data);

    if (self->new_line)
    {
        int d = *(int *)tim_inetd_service_connection((tim_inetd_service_t *)srv)->fd;
        linenoiseEditStart(&self->ls, d, d, self->line_buffer, sizeof(self->line_buffer),
                           tim_tcl_shell_prompt(self));
    }

    char *line = linenoiseEditFeed(&self->ls);
    if (line == linenoiseEditMore)
        return true;
    linenoiseEditStop(&self->ls);
    if (!line) // Ctrl+D pressed.
        exit(0);

    TIM_TRACE(Debug, "Line to process: %s", line);

    linenoiseFree(line);

    return true;
}
