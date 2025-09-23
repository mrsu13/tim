#pragma once

#include <cstdint>
#include <memory>
#include <string>


namespace tim
{

namespace p
{

struct service;

}

class service
{

public:

    virtual ~service();

    std::uint64_t id() const;
    const std::string &name() const;

protected:

    explicit service(const std::string &name);

private:

    std::unique_ptr<tim::p::service> _d;
};

}
