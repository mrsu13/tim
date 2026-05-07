#include "tim_inetd.h"


// Защищённые

tim::inetd::inetd(const std::string &name)
    : tim::service(name)
{
}


// Открытые

tim::inetd::~inetd() = default;
