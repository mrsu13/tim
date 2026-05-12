#include "tim_a_io_device.h"


/**
 * Удобный перегруз write() для std::string. Пустые строки не пишутся —
 * write(nullptr, 0) на некоторых транспортах не определён.
 */
bool tim::a_io_device::write_str(const std::string &s)
{
    return s.empty()
                ? true
                : write(s.c_str(), s.size());
}
