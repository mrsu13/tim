#pragma once

#include <cstdint>
#include <string>


namespace tim::p
{

/**
 * Внутреннее состояние tim::service (PIMPL).
 */
struct service
{
    /**
     * \return Следующий уникальный идентификатор сервиса (монотонно
     *         возрастающий счётчик).
     */
    static std::uint64_t next_id();

    /** Уникальный идентификатор сервиса. */
    std::uint64_t _id = 0;
    /** Имя сервиса (используется в логах). */
    std::string _name;
};

}
