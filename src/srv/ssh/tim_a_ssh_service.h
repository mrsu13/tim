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

    a_ssh_service(const std::string &name, ssh_session session);
    virtual ~a_ssh_service();

    tim::byte_vector &outgoing_data();
    std::size_t send_data(tim::byte_vector &&data);
    std::size_t send_string(const std::string &s);

    std::size_t data_size() const;
    std::size_t take_data(tim::byte_vector &data);

    bool flush();
    bool dispatch();
    bool running() const;
    void terminate();

private:

    std::unique_ptr<tim::p::a_ssh_service> _d;
};

}
