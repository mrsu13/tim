#pragma once

#include "tim_uuid.h"

#include <memory>


namespace tim
{

namespace p
{

struct user;

}

class user
{

public:

    explicit user(const std::string &pkey);

    const tim::uuid &id() const;
    const std::string &pkey() const;
    const std::string &nick() const;
    const std::string &icon() const;
    const std::string &motto() const;

private:

    std::unique_ptr<tim::p::user> _d;
};

}
