#pragma once

#include <stdbool.h>


typedef struct tim_app
{
    bool quit;
    void (*banner)(void * /* data */);
    void (*status)(void * /* data */);
} tim_app_t;
