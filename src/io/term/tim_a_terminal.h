#pragma once

#include "tim_a_io_device.h"
#include "tim_color.h"
#include "tim_vt_theme.h"

#include <cstdarg>


struct mg_connection;

namespace tim
{

namespace p
{

struct a_terminal;

}

class a_terminal : public tim::a_io_device
{

public:

    virtual ~a_terminal() = default;

    const tim::vt_theme &theme() const;
    void set_theme(const tim::vt_theme &theme);

    virtual std::size_t rows() const = 0;
    virtual std::size_t cols() const = 0;
    virtual void clear() = 0;

    virtual std::size_t color_count() const = 0;
    virtual tim::color color(std::uint8_t index) const = 0;

    virtual void set_color(const tim::color &c) = 0;
    virtual void set_color(std::uint8_t index) = 0;
    virtual void set_default_color() = 0;
    virtual void set_bg_color(const tim::color &c) = 0;
    virtual void set_bg_color(std::uint8_t index) = 0;
    virtual void reverse_colors() = 0;
    virtual void reset_colors() = 0;

    int vprintf(const char *format, va_list args);
    int printf(const char *format, ... )
               __attribute__ ((format(printf, 2, 3)));

    int cprintf(const tim::color &text_color,
                const tim::color &bg_color,
                const char *format, ... )
                __attribute__ ((format(printf, 4, 5)));

    virtual bool process_data(const char *data, std::size_t size) = 0;

protected:

    explicit a_terminal(mg_connection *c);

private:

    bool ready_read(const char *data, std::size_t size, std::size_t *bytes_read = nullptr);
    bool ready_write(const char *data, std::size_t size, std::size_t *bytes_written = nullptr);

    std::unique_ptr<tim::p::a_terminal> _d;
};

}
