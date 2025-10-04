#pragma once

#include <libssh/server.h>


namespace tim
{

namespace p
{

struct sshd
{
    bool on_new_session();
    void on_session_authenticated();

    std::uint16_t _port = 0;
    ssh_bind _bind;

    using session_vector = std::vector<std::unique_ptr<tim::ssh_session>>;
    session_vector _sessions;
};

}

}
