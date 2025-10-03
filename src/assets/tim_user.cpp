#include "tim_user.h"


// Public

std::string tim::user::title() const
{
    return icon.empty()
                ? std::string{}
                : icon
            + (nick.empty()
                ? std::string{}
                : ' ' + nick);
}
