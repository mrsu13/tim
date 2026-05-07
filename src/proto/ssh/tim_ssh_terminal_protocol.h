#pragma once

#include "tim_a_terminal_protocol.h"


namespace tim
{

class a_ssh_inetd_service;

// Терминальный протокол поверх SSH-канала. В отличие от telnet, никакой
// IAC-децикл/негоциация не нужна — байты в канале уже расшифрованы libssh.
// term/rows/cols берутся из самого SSH-сервиса (он получает их через
// pty-req и window-change).
class ssh_terminal_protocol : public tim::a_terminal_protocol
{

public:

    explicit ssh_terminal_protocol(tim::a_ssh_inetd_service *io);
    ~ssh_terminal_protocol();

    const std::string &terminal_name() const override;
    std::size_t rows() const override;
    std::size_t cols() const override;

    bool write(const char *data, std::size_t size) override;
    void process_raw_data(const char *data, std::size_t size) override;

private:

    tim::a_ssh_inetd_service *_ssh;
};

}
