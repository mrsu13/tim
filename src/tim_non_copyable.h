#pragma once

namespace tim
{

class non_copyable
{

protected:

    constexpr non_copyable() = default;
    non_copyable(const non_copyable &other) = delete;
    ~non_copyable() = default;

    non_copyable &operator=(const non_copyable &other) = delete;
};

}
