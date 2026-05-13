#include "tim_a_ssh_inetd_service.h"

#include <libssh/libssh.h>

#include <cassert>


// Открытые

/**
 * Закрывает SSH-канал и сессию. Идемпотентно.
 */
void tim::a_ssh_inetd_service::close()
{
    if (_channel)
    {
        if (ssh_channel_is_open(_channel) && !ssh_channel_is_closed(_channel))
            ssh_channel_send_eof(_channel);
        ssh_channel_close(_channel);
    }
}

/**
 * Возвращает срез накопленных в _recv_buf байтов.
 *
 * \param data Сюда записывается указатель на буфер.
 * \return Количество байт в буфере; после чтения буфер очищается.
 */
std::size_t tim::a_ssh_inetd_service::read(const char **data)
{
    assert(data);

    *data = _recv_buf.data();
    const std::size_t size = _recv_buf.size();
    _recv_buf.clear();
    return size;
}

/**
 * Пишет байты в SSH-канал.
 *
 * \param data Указатель на данные.
 * \param size Размер данных в байтах.
 * \return true при успехе.
 */
bool tim::a_ssh_inetd_service::write(const char *data, std::size_t size)
{
    assert(data);

    if (!size || !_channel)
        return true;

    const int rc = ssh_channel_write(_channel, data, (uint32_t)size);
    return rc == (int)size;
}

/** \return UUID пользователя сессии. */
const tim::uuid &tim::a_ssh_inetd_service::user_id() const noexcept
{
    return _user_id;
}

/** \return Открытый ключ клиента (OpenSSH-формат). */
const std::string &tim::a_ssh_inetd_service::pub_key() const noexcept
{
    return _pub_key;
}

/** \return $TERM клиента. */
const std::string &tim::a_ssh_inetd_service::term_name() const noexcept
{
    return _term_name;
}

/** \return Высота терминала клиента в строках. */
std::size_t tim::a_ssh_inetd_service::rows() const noexcept
{
    return _rows;
}

/** \return Ширина терминала клиента в колонках. */
std::size_t tim::a_ssh_inetd_service::cols() const noexcept
{
    return _cols;
}

/**
 * Вызывается ssh_inetd при поступлении байтов в канал. Кладёт
 * их во внутренний буфер и испускает сигнал data_ready (через
 * a_io_device).
 *
 * \param data Указатель на принятые байты.
 * \param size Размер принятых данных.
 */
void tim::a_ssh_inetd_service::on_channel_data(const char *data, std::size_t size)
{
    if (!size)
        return;

    _recv_buf.insert(_recv_buf.end(), data, data + size);
    ready_read();
}

/**
 * Вызывается ssh_inetd при изменении размеров терминала клиента.
 *
 * \param rows Новая высота в строках.
 * \param cols Новая ширина в колонках.
 */
void tim::a_ssh_inetd_service::on_window_change(std::size_t rows, std::size_t cols) noexcept
{
    _rows = rows;
    _cols = cols;
}


// Защищённые

/**
 * Конструктор только для наследников.
 *
 * \param name Имя сервиса.
 * \param info Параметры открытой SSH-сессии.
 */
tim::a_ssh_inetd_service::a_ssh_inetd_service(const std::string &name, const tim::ssh_session_info &info)
    : tim::a_inetd_service(name)
    , _session(info.session)
    , _channel(info.channel)
    , _user_id(info.user_id)
    , _pub_key(info.pub_key)
    , _term_name(info.term_name)
    , _rows(info.rows)
    , _cols(info.cols)
{
    assert(_session);
    assert(_channel);
}

/** Деструктор для наследников. */
tim::a_ssh_inetd_service::~a_ssh_inetd_service() = default;
