#pragma once

#include "mongoose.h"

#include <stdbool.h>


typedef struct tim_app
{
    bool quit;

    struct mg_mgr mg;
} tim_app_t;
