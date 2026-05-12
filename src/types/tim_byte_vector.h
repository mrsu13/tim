#pragma once

#include <vector>
#include <cstdint>


namespace tim
{

/**
 * Псевдоним для контейнера сырых байтов. Используется везде, где
 * нужно явно подчеркнуть бинарную природу данных (TLS-сертификаты,
 * UUID-байты, IO-буферы).
 */
using byte_vector = std::vector<std::uint8_t>;

}
