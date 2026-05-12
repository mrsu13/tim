#include "tim_a_protocol.h"

#include "tim_a_protocol_p.h"

#include "tim_a_io_device.h"

#include <cassert>


// Public

/**
 * Конструктор. Подписывает on_ready_read() на сигнал ready_read
 * транспорта \a io.
 */
tim::a_protocol::a_protocol(tim::a_io_device *io)
    : data_ready()
    , _d(new tim::p::a_protocol(this))
{
    assert(io);

    _d->_io = io;
    // ready_read → on_ready_read → process_raw_data (через _q).
    _d->_on_ready_read = _d->_io->ready_read.connect(
        [d = _d.get()]{ d->on_ready_read(); });
}

/** Деструктор. PIMPL разрушает _d, _on_ready_read отписывается RAII-ем. */
tim::a_protocol::~a_protocol() = default;

/** \return Указатель на транспорт, на который подписан протокол. */
tim::a_io_device *tim::a_protocol::io() const
{
    return _d->_io;
}

/**
 * Удобный перегруз write() для std::string. Пустые строки тихо
 * пропускаются (write на нулевой буфер не имеет смысла).
 */
bool tim::a_protocol::write_str(const std::string &s)
{
    return s.empty()
                ? true
                : write(s.c_str(), s.size());
}


// Private

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
