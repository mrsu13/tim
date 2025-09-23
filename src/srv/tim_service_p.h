#pragma once

#include <cstdint>
#include <string>


namespace tim::p
{

struct service
{
    static std::uint64_t next_id();

    std::uint64_t _id = 0;
    std::string _name;
};

}
