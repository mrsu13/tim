#pragma once

#include "tim_ssh_session.h"

#include <co_io_server.h>

#include <libssh/server.h>

#include <list>


namespace tim
{
    
class ssh_session;

class ssh_server : public co::io_server
{

public:

    CO_SIGNAL(new_session, tim::ssh_session *);

    ssh_server(std::uint16_t port, co::object *parent = nullptr);
    virtual ~ssh_server();

    inline std::uint16_t port() const;

    bool has_session(const std::string &imei) const;

    co::rc dispatch();

private:

    co::rc init();
    co::rc on_new_session();
    void on_session_authenticated();

    std::uint16_t _port;
    ssh_bind _bind;

    using session_list = std::list<std::unique_ptr<tim::ssh_session>>;
    session_list _sessions;
};

}


// Implementation

// Public Methods

inline std::uint16_t tim::ssh_server::port() const
{
    return _port;
}
