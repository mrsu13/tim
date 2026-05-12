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

/**
 * inetd поверх libssh.
 *
 * Открывает host key, слушает SSH-порт, аутентифицирует клиента
 * по открытому ключу (любой ключ принимается, его SHA-256 становится
 * идентификатором пользователя), и при первом запросе shell-а вызывает
 * фабрику для создания сервиса, обслуживающего канал.
 */
class ssh_inetd : public tim::inetd
{

public:

    /**
     * Тип фабрики сервисов: получает параметры сессии и возвращает
     * новый a_inetd_service для обслуживания канала.
     */
    using service_factory = std::function<std::unique_ptr<tim::a_inetd_service>(const tim::ssh_session_info &)>;

    /** Деструктор. Закрывает все живые сессии и освобождает ssh_bind. */
    ~ssh_inetd();

    /**
     * Создаёт и запускает inetd. При ошибке открытия host key или
     * прослушивания порта возвращает nullptr (логирует error).
     *
     * \param port TCP-порт для прослушивания.
     * \param host_key_path Путь к ed25519 host-key. Если отсутствует —
     *                     генерируется при первом запуске.
     * \param factory Фабрика сервисов: создаёт прикладной сервис
     *                для каждой аутентифицированной сессии.
     * \param if_addr Интерфейс для bind (пусто = все).
     * \return RAII-указатель на работающий inetd; nullptr при ошибке.
     */
    static std::unique_ptr<tim::ssh_inetd> start(std::uint16_t port,
                                                 const std::filesystem::path &host_key_path,
                                                 service_factory factory,
                                                 const std::string &if_addr = "");

    /**
     * Опрашивает libssh-события (приём новых соединений, рукопожатие,
     * аутентификацию, обмен данными). Должна вызываться периодически
     * из главного цикла приложения.
     *
     * Re-entrant safe: внутренний счётчик dispatch_depth откладывает
     * освобождение сессий до самого внешнего уровня (Tcl-скрипт может
     * рекурсивно дёрнуть dispatch через DISPATCH-обработчик).
     *
     * \param timeout_ms Максимальное время блокировки в миллисекундах.
     */
    void dispatch(int timeout_ms);

    /**
     * Прерывает работу всех живых сервисов (interrupt() на каждом).
     *
     * Нужно на завершении сервера, чтобы долгий /while 1 {} не
     * блокировал выход из exec().
     */
    void interrupt_all();

private:

    /**
     * Закрытый конструктор; экземпляры создаются только через start().
     *
     * \param port TCP-порт.
     * \param host_key_path Путь к host-key.
     * \param if_addr Интерфейс bind.
     * \param factory Фабрика сервисов.
     */
    ssh_inetd(std::uint16_t port,
              const std::filesystem::path &host_key_path,
              const std::string &if_addr,
              service_factory factory);

    /** PIMPL: ssh_bind, ssh_event, карта сессий, счётчик вложенности. */
    std::unique_ptr<tim::p::ssh_inetd> _d;
};

}
