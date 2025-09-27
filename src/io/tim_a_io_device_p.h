#pragma once

#include "tim_a_io_device.h"


struct mg_connection;

namespace tim::p
{

struct a_io_device
{
    mg_connection *_c = nullptr;
    tim::a_io_device::data_handler _data_handler;
};

}
