#include "tim_a_io_device.h"


bool tim::a_io_device::write_str(const std::string &s)
{
    return s.empty()
                ? true
                : write(s.c_str(), s.size());
}
