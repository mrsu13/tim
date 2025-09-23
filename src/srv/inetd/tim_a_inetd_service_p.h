#pragma once

struct mg_connection;

namespace tim::p
{

struct a_inetd_service
{
    mg_connection *_c = nullptr;
};

}
