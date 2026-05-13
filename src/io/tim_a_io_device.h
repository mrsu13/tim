#pragma once

#include "tim_signal.h"

#include <cstddef>
#include <string>


namespace tim
{

/**
 * Транспорт-независимый интерфейс байтового ввода/вывода.
 *
 * Используется поверх SSH-канала, TCP-сокета, файла и т.п. Подписчики
 * слушают сигнал ready_read; при появлении новых байтов вызывают read()
 * для забора.
 */
class a_io_device
{

public:

    /** Испускается транспортом, когда у него появились байты для чтения. */
    tim::signal<> ready_read;

    /** Виртуальный деструктор. */
    virtual ~a_io_device() = default;

    /** Закрывает устройство. После close() read/write возвращают ошибку. */
    virtual void close() = 0;

    /**
     * Забирает прочитанные транспортом байты.
     *
     * \param data Сюда записывается указатель на буфер с байтами.
     *             Указатель валиден до следующего вызова read/close.
     * \return Размер буфера; 0 — нет данных или устройство закрыто.
     */
    virtual std::size_t read(const char **data) = 0;

    /**
     * Пишет байты в устройство.
     *
     * \param data Указатель на данные.
     * \param size Размер в байтах.
     * \return true при успехе.
     */
    virtual bool write(const char *data, std::size_t size) = 0;

    bool write_str(const std::string &s);
};

}
