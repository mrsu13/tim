#include "tim_a_protocol.h"

#include "tim_a_protocol_p.h"

#include "tim_a_io_device.h"

#include <cassert>


// Открытые

/**
 * Конструктор. Подключается к ready_read у \a io.
 *
 * \param io Транспорт; должен жить дольше протокола.
 */
tim::a_protocol::a_protocol(tim::a_io_device *io)
    : data_ready()
    , _d(this)
{
    assert(io);

    _d->_io = io;
    // ready_read → on_ready_read → process_raw_data (через _q).
    _d->_on_ready_read = _d->_io->ready_read.connect(
        [d = _d.get()]{ d->on_ready_read(); });
}

/** Виртуальный деструктор. */
tim::a_protocol::~a_protocol() = default;

/** \return Указатель на транспорт, на который подписан протокол. */
tim::a_io_device *tim::a_protocol::io() const
{
    return _d->_io;
}

/**
 * Сахар write() для std::string.
 *
 * \param s Строка для записи.
 * \return true при успехе.
 */
bool tim::a_protocol::write_str(const std::string &s)
{
    return s.empty()
                ? true
                : write(s.c_str(), s.size());
}


// Закрытые

/**
 * Читает накопленные транспортом байты и передаёт во внешний
 * process_raw_data() через _q (указатель на внешний a_protocol).
 */
void tim::p::a_protocol::on_ready_read()
{
    const char *data;
    const std::size_t size = _io->read(&data);
    _q->process_raw_data(data, size);
}
