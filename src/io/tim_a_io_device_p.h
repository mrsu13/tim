#pragma once

struct mg_connection;

namespace tim::p
{

struct a_io_device
{
    mg_connection *_c = nullptr;
};

}
