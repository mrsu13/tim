#include "tim_user.h"


// Открытые

std::string tim::user::title() const
{
    if (!icon.empty() && !nick.empty())
        return icon + ' ' + nick;
    if (!nick.empty())
        return nick;
    if (!icon.empty())
        return icon;
    return id.valid()
                ? id.to_string(tim::uuid::format::Compact).substr(0, 8)
                : std::string{};
}
