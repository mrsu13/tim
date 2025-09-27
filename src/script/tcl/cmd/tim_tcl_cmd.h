#pragma once

#include <cassert>


#define TIM_TCL_REGISTER(lil, name) \
if (lil_register((lil), #name, tim_tcl_cmd_##name) <= 0) \
    return assert(0 && "Failed to register TCL command '" #name "'.");
