#pragma once

#include "tim_a_terminal.h"

#include <string>


namespace tim
{

class a_terminal_protocol;

namespace p
{

struct vt;

}

/**
 * VT100/xterm-эмулятор: реализация a_terminal через ANSI escape-sequences.
 *
 * Управление цветами, перемещением курсора и очисткой экрана выполняется
 * через стандартные ESC-последовательности; терминал клиента сам их
 * интерпретирует.
 */
class vt : public tim::a_terminal
{

public:

    /**
     * Конструктор.
     *
     * \param proto Терминальный протокол (источник $TERM/rows/cols
     *              и приёмник байт).
     */
    explicit vt(tim::a_terminal_protocol *proto);

    /** Деструктор. */
    ~vt();

    /** \return Высота окна в строках (берётся из proto). */
    std::size_t rows() const override;
    /** \return Ширина окна в колонках (берётся из proto). */
    std::size_t cols() const override;
    /** Очищает экран и переводит курсор в (1,1). */
    void clear() override;

    /** \return Размер палитры терминала (256 для xterm-256color). */
    std::size_t color_count() const override;
    /**
     * Цвет из палитры терминала по индексу.
     *
     * \param index 0..color_count()-1.
     * \return RGBA-цвет.
     */
    tim::color color(std::size_t index) const override;
    /** Устанавливает цвет текста (RGB). \param c Цвет. */
    void set_color(const tim::color &c) override;
    /** Устанавливает цвет текста по индексу палитры. \param index Индекс. */
    void set_color(std::size_t index) override;
    /** Возвращает цвет текста к default-цвету темы. */
    void set_default_color() override;
    /** Устанавливает цвет фона (RGB). \param c Цвет. */
    void set_bg_color(const tim::color &c) override;
    /** Устанавливает цвет фона по индексу палитры. \param index Индекс. */
    void set_bg_color(std::size_t index) override;
    /** Меняет местами текст и фон. */
    void reverse_colors() override;
    /** Сбрасывает цвета и атрибуты. */
    void reset_colors() override;

    /**
     * Возвращает строку с ANSI-обвязкой цветов (для отложенного вывода
     * через printf и сохранения в буфер).
     *
     * \param s Исходная строка.
     * \param text_color Цвет текста.
     * \param bg_color Цвет фона (пустой/transparent — без фона).
     * \return Строка с ESC-кодами вокруг \a s.
     */
    static std::string colorized(const std::string &s,
                                 const tim::color &text_color,
                                 const tim::color &bg_color = tim::color{});

    /**
     * Длина строки без учёта ANSI ESC-кодов (visual-aware).
     *
     * \param s Строка, возможно содержащая ESC-обвязку.
     * \return Число видимых символов.
     */
    static std::size_t strlen(const std::string &s);

private:

    /** PIMPL: цвета, буферы, состояние ANSI-state machine. */
    std::unique_ptr<tim::p::vt> _d;
};

}
