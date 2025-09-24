#pragma once

#include "tim_a_inetd_service.h"
#include "tim_color.h"
#include "tim_vt_theme.h"

#include <cstdarg>


namespace tim
{

namespace p
{

struct a_telnet_service;

}

class a_telnet_service : public tim::a_inetd_service
{

public:

    ~a_telnet_service();

    std::size_t rows() const;
    std::size_t cols() const;
    void clear();

    const tim::vt_theme &theme() const;
    void set_theme(const tim::vt_theme &theme);

    static tim::color color(std::uint8_t index);
    void set_color(const tim::color &c);
    void set_color(std::uint8_t index);
    void set_default_color();
    void set_bg_color(const tim::color &c);
    void set_bg_color(std::uint8_t index);
    void reverse_colors();
    void reset_colors();
    static std::string colorized(const std::string &s,
                                 const tim::color &text_color,
                                 const tim::color &bg_color = tim::color{});
    static std::size_t strlen(const std::string &s);

    int vprintf(const char *format, va_list args);
    int printf(const char *format, ... )
               __attribute__ ((format(printf, 2, 3)));

    virtual bool process_data(const char *data, std::size_t size) = 0;

protected:

    explicit a_telnet_service(const std::string &name, mg_connection *c);

private:

    bool ready_read(const char *data, std::size_t size, std::size_t *bytes_read = nullptr);
    bool ready_write(const char *data, std::size_t size, std::size_t *bytes_written = nullptr);

    std::unique_ptr<tim::p::a_telnet_service> _d;
};

}
