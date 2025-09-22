#include "tim_tcl_shell.h"

#include "tim_telnet_service.h"
#include "tim_telnet_service_p.h"

#include "tim_inetd_service.h"

#include "tcl.h"

#include <assert.h>
#include <stdlib.h>


typedef struct tim_tcl_shell
{
    tim_telnet_service_t super;
    struct tcl tcl;
} tim_tcl_shell_t;

static const char *tim_tcl_shell_welcome_banner();
static const char *tim_tcl_shell_bye_banner();
static const char *tim_tcl_shell_prompt();

tim_tcl_shell_t *tim_tcl_shell_new(struct mg_connection *c)
{
    tim_tcl_shell_t *srv = (tim_tcl_shell_t *)calloc(1, sizeof(tim_tcl_shell_t));
    assert(srv);

    tim_telnet_service_init(&srv->super, c);
    tcl_init(&srv->tcl);

    tim_inetd_service_write_str((tim_inetd_service_t *)srv, tim_tcl_shell_welcome_banner());
    tim_inetd_service_write_str((tim_inetd_service_t *)srv, tim_tcl_shell_prompt());

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

const char *tim_tcl_shell_prompt()
{
    return ">";
}
