#pragma once

#include "mongoose.h"

#include <stdbool.h>


typedef struct tim_inetd tim_inetd_t;

typedef struct tim_app
{
    bool quit;

    struct mg_mgr mg;
    tim_inetd_t *tcl_shell_inetd;
} tim_app_t;
