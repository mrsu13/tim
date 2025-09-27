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
    ~user();

    user(const &tim::user &other) = delete;
    tim::user &operator(const tim::user &other) = delete;

    const tim::uuid &id() const;
    const std::string &pkey() const;
    const std::string &nick() const;
    const std::string &icon() const;
    const std::string &motto() const;

private:

    // std::unique_ptr<tim::p::user> _d;
};

std::shared_ptr<tim::user> p(new tim::user());

std::weak_ptr<tim::user> wp(p);
std::shared_ptr<tim::user> p1 = wp.lock();
if (!p1)
    p1->do_something();

user user1 = user2;
}
