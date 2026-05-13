#pragma once

#include "tim_terminal_theme.h"

#include <cstdarg>
#include <memory>


namespace tim
{

class a_protocol;

namespace p
{

struct a_terminal;

}

/**
 * Абстрактный терминал: интерфейс над a_protocol для вывода форматированного
 * текста, управления цветами/темой и получения геометрии окна.
 *
 * Конкретная реализация — tim::vt (VT100-emulator).
 */
class a_terminal
{

public:

    virtual ~a_terminal();

    tim::a_protocol *protocol() const;

    const tim::terminal_theme &theme() const;

    void set_theme(const tim::terminal_theme &theme);

    /** \return Высота окна в строках. */
    virtual std::size_t rows() const = 0;
    /** \return Ширина окна в колонках. */
    virtual std::size_t cols() const = 0;
    /** Очищает экран (ANSI ESC [2J). */
    virtual void clear() = 0;

    /** \return Количество поддерживаемых индексированных цветов. */
    virtual std::size_t color_count() const = 0;
    /**
     * Цвет по индексу из встроенной палитры.
     *
     * \param index 0-based индекс.
     * \return RGBA-цвет.
     */
    virtual tim::color color(std::size_t index) const = 0;

    /** Устанавливает цвет текста (RGB). \param c Цвет. */
    virtual void set_color(const tim::color &c) = 0;
    /** Устанавливает цвет текста по индексу палитры. \param index Индекс. */
    virtual void set_color(std::size_t index) = 0;
    /** Возвращает цвет текста к default. */
    virtual void set_default_color() = 0;
    /** Устанавливает цвет фона (RGB). \param c Цвет. */
    virtual void set_bg_color(const tim::color &c) = 0;
    /** Устанавливает цвет фона по индексу палитры. \param index Индекс. */
    virtual void set_bg_color(std::size_t index) = 0;
    /** Меняет местами текст и фон (ANSI ESC [7m). */
    virtual void reverse_colors() = 0;
    /** Сбрасывает все цвета и атрибуты (ANSI ESC [0m). */
    virtual void reset_colors() = 0;

    int vprintf(const char *format, va_list args);

    int printf(const char *format, ... )
               __attribute__ ((format(printf, 2, 3)));

    int cprintf(const tim::color &text_color,
                const tim::color &bg_color,
                const char *format, ... )
                __attribute__ ((format(printf, 4, 5)));

protected:

    explicit a_terminal(tim::a_protocol *proto);

private:

    /** PIMPL: указатель на протокол и текущая тема. */
    std::unique_ptr<tim::p::a_terminal> _d;
};

}
