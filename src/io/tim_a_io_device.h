#pragma once

#include "tim_signal.h"

#include <cstddef>
#include <string>


namespace tim
{

// Транспорт-независимый интерфейс байтового ввода/вывода.
// Сигнал ready_read() вызывается, когда у транспорта появились новые
// прочитанные байты для обработчика.
class a_io_device
{

public:

    tim::signal<> ready_read;

    virtual ~a_io_device() = default;

    virtual void close() = 0;
    virtual std::size_t read(const char **data) = 0;
    virtual bool write(const char *data, std::size_t size) = 0;

    bool write_str(const std::string &s);
};

}
