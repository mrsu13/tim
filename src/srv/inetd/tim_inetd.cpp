#include "tim_inetd.h"


// Защищённые

/**
 * Конструктор только для наследников.
 *
 * \param name Имя сервиса (используется в логах).
 */
tim::inetd::inetd(const std::string &name)
    : tim::service(name)
{
}


// Открытые

/** Виртуальный деструктор для полиморфного удаления. */
tim::inetd::~inetd() = default;
