#pragma once

#include "tim_signal.h"

#include "tim_pimpl.h"


namespace tim
{

class a_io_device;

namespace p
{

struct a_protocol;

}

/**
 * Абстрактный байтовый протокол поверх a_io_device.
 *
 * Получает сырые байты через process_raw_data() (которую переопределяют
 * конкретные реализации — telnet, ssh-terminal и т.д.) и испускает сигнал
 * data_ready с "очищенными" пользовательскими данными.
 */
class a_protocol
{

public:

    /** Сигнал: накоплены пользовательские данные. \a data, \a size. */
    tim::signal<const char * /* data */, std::size_t /* size */> data_ready;

    a_protocol(tim::a_io_device *io);

    virtual ~a_protocol();

    tim::a_io_device *io() const;

    /**
     * Кодирует и пишет данные в транспорт (например, экранирует IAC
     * для telnet или ничего не делает для ssh).
     *
     * \param data Указатель на данные.
     * \param size Размер.
     * \return true при успехе.
     */
    virtual bool write(const char *data, std::size_t size) = 0;

    bool write_str(const std::string &s);

    /**
     * Обработать сырые байты, пришедшие из транспорта. Реализуется
     * наследниками; они должны при необходимости испустить data_ready.
     *
     * \param data Сырые байты.
     * \param size Размер.
     */
    virtual void process_raw_data(const char *data, std::size_t size) = 0;

private:

    /** PIMPL: подключение к ready_read и указатель на io. */
    tim::pimpl<tim::p::a_protocol> _d;
};

}
