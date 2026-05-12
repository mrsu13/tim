#pragma once

#include "tim_a_terminal_protocol.h"


namespace tim
{

class a_ssh_inetd_service;

/**
 * Терминальный протокол поверх SSH-канала.
 *
 * В отличие от telnet, никакой IAC-децикл/негоциация не нужна — байты
 * в канале уже расшифрованы libssh. term/rows/cols берутся из самого
 * SSH-сервиса (он получает их через pty-req и window-change).
 */
class ssh_terminal_protocol : public tim::a_terminal_protocol
{

public:

    /**
     * Конструктор.
     *
     * \param io SSH-inetd-сервис; источник байт и параметров терминала.
     */
    explicit ssh_terminal_protocol(tim::a_ssh_inetd_service *io);

    /** Деструктор. */
    ~ssh_terminal_protocol();

    /** \return Имя терминала клиента (значение $TERM). */
    const std::string &terminal_name() const override;
    /** \return Высота окна клиента в строках. */
    std::size_t rows() const override;
    /** \return Ширина окна клиента в колонках. */
    std::size_t cols() const override;

    /**
     * Пишет данные в SSH-канал (через _ssh->write()).
     *
     * \param data Указатель на данные.
     * \param size Размер.
     * \return true при успехе.
     */
    bool write(const char *data, std::size_t size) override;

    /**
     * Передаёт сырые байты пользователю без обработки — для SSH-канала
     * протокольный layer не нужен. Испускает data_ready напрямую.
     *
     * \param data Сырые байты.
     * \param size Размер.
     */
    void process_raw_data(const char *data, std::size_t size) override;

private:

    /** SSH-сервис, источник байт и параметров. */
    tim::a_ssh_inetd_service *_ssh;
};

}
