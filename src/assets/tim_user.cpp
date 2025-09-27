#include "tim_user.h"

#include "tim_user_p.h"

#include <cassert>


// Public

tim::user::user(const std::string &pkey)
    : _d(new tim::p::user())
{
    _d->_pkey = pkey;

    assert(!_d->_pkey.empty() && "User's public key must not be empty.");
}

tim::user::~user() = default;

const tim::uuid &tim::user::id() const
{
    return _d->_id;
}

const std::string &tim::user::pkey() const
{
    return _d->_pkey;
}

const std::string &tim::user::nick() const
{
    return _d->_nick;
}

const std::string &tim::user::icon() const
{
    return _d->_icon;
}
const std::string &tim::user::motto() const
{
    return _d->_motto;
}
