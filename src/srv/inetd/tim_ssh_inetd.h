#pragma once

#include "tim_inetd.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>


namespace tim
{

namespace p
{

struct ssh_inetd;

}

class a_inetd_service;
struct ssh_session_info;

// inetd поверх libssh: открывает host key, слушает SSH-порт, аутентифицирует
// клиента по открытому ключу (любой ключ принимается, его SHA-256 становится
// идентификатором пользователя), и при первом запросе shell-а вызывает
// фабрику для создания сервиса, обслуживающего канал.
class ssh_inetd : public tim::inetd
{

public:

    using service_factory = std::function<std::unique_ptr<tim::a_inetd_service>(const tim::ssh_session_info &)>;

    ~ssh_inetd();

    static std::unique_ptr<tim::ssh_inetd> start(std::uint16_t port,
                                                 const std::filesystem::path &host_key_path,
                                                 service_factory factory,
                                                 const std::string &if_addr = "");

    // Опросить libssh-события (приём новых соединений, рукопожатие, аутентификацию,
    // обмен данными). Должна вызываться периодически из главного цикла приложения.
    void dispatch(int timeout_ms);

    // Прерывает работу всех живых сервисов (например, выполняемые
    // Tcl-скрипты). Нужно на завершении сервера, чтобы долгий /while 1 {}
    // не блокировал выход из exec().
    void interrupt_all();

private:

    ssh_inetd(std::uint16_t port,
              const std::filesystem::path &host_key_path,
              const std::string &if_addr,
              service_factory factory);

    std::unique_ptr<tim::p::ssh_inetd> _d;
};

}
