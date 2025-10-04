#pragma once

#include "tim_ssh_session.h"

#include <libssh/server.h>


namespace tim
{

namespace p
{

struct sshd;

}

class sshd : public tim::service
{

public:

    using service_factory = std::function<std::unique_ptr<tim::a_ssh_service>(ssh_session session)>;

    virtual ~sshd();

    template<class S>
    inline static std::unique_ptr<tim::sshd> start(mg_mgr *mg,
                                                   std::uint16_t port,
                                                   const std::string &if_addr = "");

    bool dispatch();

private:

    sshd(mg_mgr *mg,
         std::uint16_t port,
         const std::string &if_addr,
         service_factory factory);

    std::unique_ptr<tim::p::sshd> _d;
};

}


// Implementation

// Public

template<class S>
std::unique_ptr<tim::sshd> tim::sshd::start(mg_mgr *mg,
                                            std::uint16_t port,
                                            const std::string &if_addr)
{
    static_assert(std::is_base_of_v<tim::a_ssh_service, S>,
                  "S must be a descendant of tim::a_ssh_service class.");

    return std::unique_ptr<tim::sshd>(
                new tim::sshd(mg, port, if_addr,
                              [](ssh_session session)
                              {
                                  return std::make_unique<S>(session);
                              }));
}
