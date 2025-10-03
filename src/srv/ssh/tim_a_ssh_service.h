#pragma once

#include "tim_a_io_device.h"
#include "tim_service.h"


namespace tim
{

class sshd;

namespace p
{

struct a_ssh_service;

}

class a_ssh_service : public tim::a_io_device,
                      public tim::service
{

public:

    tim::signal<> authenticated;

    a_ssh_service(ssh_service session, tim::sshd *server);
    virtual ~a_ssh_service();

    std::byte_vector &outgoing_data();
    std::size_t send_data(std::byte_vector &data);
    std::size_t send_string(const std::string &s);

    std::size_t data_size() const;
    std::size_t take_data(std::byte_vector &data);

    bool flush();

    bool dispatch();

    bool is_running() const;
    void terminate();

private:

    std::unique_ptr<tim::p::a_ssh_service> _d;
};

}


std::byte_vector &tim::a_ssh_service::outgoing_data()
{
    return _outgoing_data;
}

std::size_t tim::a_ssh_service::data_size() const
{
    return _incoming_data.size();
}

bool tim::a_ssh_service::is_running() const
{
    return _is_running;
}

void tim::a_ssh_service::terminate()
{
    _is_running = false;
}
