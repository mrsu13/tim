#pragma once

#include "tim_a_protocol.h"


namespace tim
{

/**
 * Расширение a_protocol для терминальных транспортов: добавляет имя
 * терминала и размеры окна. Конкретные реализации (telnet, ssh-terminal)
 * заполняют их из своих специфичных каналов сигнализации.
 */
class a_terminal_protocol : public tim::a_protocol
{

public:

    /**
     * Конструктор.
     *
     * \param io Транспорт; должен жить дольше протокола.
     */
    explicit a_terminal_protocol(tim::a_io_device *io);

    /** \return Тип терминала клиента ($TERM, например "xterm-256color"). */
    virtual const std::string &terminal_name() const = 0;
    /** \return Высота окна клиента в строках. */
    virtual std::size_t rows() const = 0;
    /** \return Ширина окна клиента в колонках. */
    virtual std::size_t cols() const = 0;
};

}
