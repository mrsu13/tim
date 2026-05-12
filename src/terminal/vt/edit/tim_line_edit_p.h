#pragma once

#include "tim_line_edit.h"

#include "tim_flags.h"
#include "tim_string_tools.h"

#include <cstddef>
#include <vector>


namespace tim
{

/**
 * Коды управляющих клавиш ASCII, которые line_edit обрабатывает специально.
 *
 * Значения соответствуют сырым байтам: Ctrl+A == 1, ..., Esc == 27,
 * Backspace == 127. Имя `null` (а не `Null`) выбрано для согласованности
 * с общим snake_case-стилем перечислений TIM.
 */
enum class key : char
{
    null      = 0,
    ctrl_a    = 1,
    ctrl_b    = 2,
    ctrl_c    = 3,
    ctrl_d    = 4,
    ctrl_e    = 5,
    ctrl_f    = 6,
    ctrl_h    = 8,
    tab       = 9,
    cr        = 10,
    ctrl_k    = 11,
    ctrl_l    = 12,
    enter     = 13,
    ctrl_n    = 14,
    ctrl_p    = 16,
    ctrl_t    = 20,
    ctrl_u    = 21,
    ctrl_w    = 23,
    esc       = 27,
    backspace = 127
};

class a_terminal;

namespace p
{

/**
 * Внутреннее состояние tim::line_edit (PIMPL).
 *
 * Хранит буфер, историю, колбэки автодополнения и подсказок, текущую
 * позицию курсора и параметры окна. Методы реализуют примитивы
 * редактирования и перерисовки.
 */
struct line_edit
{
    /**
     * Битовая маска состояний перерисовки приглашения.
     */
    enum class refresh_flag
    {
        clean = 1 << 0,        ///< Стереть старое приглашение с экрана.
        write = 1 << 1,        ///< Записать новое приглашение на экран.
        all   = clean | write  ///< Оба действия.
    };

    /** Битовая маска refresh_flag. */
    using refresh_flags = tim::flags<refresh_flag>;

    /**
     * Направление перемещения по истории.
     */
    enum class history_dir
    {
        prev, ///< Назад (более старая запись).
        next  ///< Вперёд (более новая запись).
    };

    /**
     * Добавляет строку в историю редактирования (с дедупликацией).
     *
     * \param line Строка для добавления.
     */
    void history_add(const std::wstring &line);

    /** Звуковой сигнал терминала (BEL). */
    void beep();

    /**
     * Перерисовывает строку с учётом списка автодополнений.
     *
     * \param c Список вариантов или nullptr.
     * \param flags Что именно перерисовать.
     */
    void refresh_line_with_completion(const tim::line_edit::completions *c, refresh_flags flags);

    /**
     * Цикл автодополнения: после Tab перебирает варианты при повторных нажатиях.
     *
     * \param key_pressed Код последней нажатой клавиши.
     * \return Кодировка результата для caller-loop.
     */
    std::int32_t complete_line(std::int32_t key_pressed);

    /**
     * Дорисовывает подсказку (hint) справа от ввода.
     *
     * \param s Строка-результат hinter_fn, сюда дописываются ANSI-коды.
     */
    void refresh_show_hints(std::wstring &s);

    /**
     * Перерисовка в однострочном режиме.
     *
     * \param flags Что перерисовать.
     */
    void refresh_single_line(refresh_flags flags);

    /**
     * Перерисовка в многострочном режиме.
     *
     * \param flags Что перерисовать.
     */
    void refresh_multi_line(refresh_flags flags);

    /**
     * Универсальная перерисовка: выбирает single или multi в зависимости
     * от _ml_mode.
     *
     * \param flags Что перерисовать.
     */
    void refresh_line_with_flags(refresh_flags flags);

    /** Перерисовка с флагами refresh_flag::all. */
    void refresh_line();

    /**
     * Вставка символа в текущую позицию.
     *
     * \param c UTF-32-код символа.
     * \return true при успехе.
     */
    bool edit_insert(std::int32_t c);

    /** Сдвиг курсора влево. */
    void edit_move_left();
    /** Сдвиг курсора вправо. */
    void edit_move_right();
    /** Курсор в начало строки (Home). */
    void edit_move_home();
    /** Курсор в конец строки (End). */
    void edit_move_end();
    /** Удалить символ под курсором (Delete). */
    void edit_delete();
    /** Удалить символ слева от курсора (Backspace). */
    void edit_backspace();
    /** Удалить слово слева (Ctrl+W). */
    void edit_delete_prev_word();

    /**
     * Перемещение по истории: подставляет следующую/предыдущую запись.
     *
     * \param dir Направление.
     */
    void edit_history_next(history_dir dir);

    /** Терминал, в котором редактируется строка. */
    tim::vt *_terminal = nullptr;

    /** Колбэк автодополнения. */
    tim::line_edit::completer_fn _completer;
    /** Колбэк подсказок. */
    tim::line_edit::hinter_fn _hinter;

    /** Пользователь нажал Tab; ввод обрабатывается через complete_line(). */
    bool _in_completion = false;
    /** Индекс следующего варианта автодополнения для предложения. */
    std::size_t _completion_idx = 0;
    /** Буфер редактируемой строки. */
    std::wstring _line;
    /** Жёсткий предел длины буфера. */
    static constexpr const std::size_t MAX_LINE_SIZE = 4096;
    /** Текущее приглашение. */
    std::wstring _prompt;
    /** Длина приглашения без учёта ANSI-обвязки. */
    std::size_t _plen = 0;
    /** Текущая позиция курсора в буфере. */
    std::size_t _pos = 0;
    /** Позиция курсора на момент прошлой перерисовки. */
    std::size_t _old_pos = 0;
    /** Ширина окна терминала в колонках. */
    std::size_t _cols = 0;
    /** Количество строк, занятых при прошлой перерисовке (multiline). */
    std::size_t _old_rows = 0;

    /** Тип контейнера истории (вектор широких строк). */
    using history = std::vector<std::wstring>;
    /** Хранилище истории. */
    history _history;
    /** Индекс текущей просматриваемой записи истории. */
    int _history_idx = 0;

    /** Маска ввода: показывать "***" вместо настоящих символов. */
    bool _mask_mode = false;
    /** Многострочный режим (по умолчанию однострочный). */
    bool _ml_mode = false;

    /** Счётчик строк — нужен, чтобы не печатать \\n при самом первом new_line(). */
    std::size_t _line_count = 0;
};

}

}
