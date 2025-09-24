#pragma once

namespace tim
{

/**
 * Languages supported by tim::tr.
 *
 * \see tim::tr TIM_TR()
 */
enum class language
{
    Unknown = 0, ///< Not defined.

    en_US = 1 << 0, ///< English USA.
    ru_RU = 1 << 1  ///< Russian.
};

}
