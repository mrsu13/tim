#pragma once

#include "tim_line_edit.h"

#include "tim_flags.h"
#include "tim_string_tools.h"

#include <cstddef>
#include <vector>


namespace tim
{

enum class key : char
{
    Null      = 0,
    Ctrl_A    = 1,
    Ctrl_B    = 2,
    Ctrl_C    = 3,
    Ctrl_D    = 4,
    Ctrl_E    = 5,
    Ctrl_F    = 6,
    Ctrl_H    = 8,
    Tab       = 9,
    Cr        = 10,
    Ctrl_K    = 11,
    Ctrl_L    = 12,
    Enter     = 13,
    Ctrl_N    = 14,
    Ctrl_P    = 16,
    Ctrl_T    = 20,
    Ctrl_U    = 21,
    Ctrl_W    = 23,
    Esc       = 27,
    Backspace = 127
};

class a_telnet_service;

namespace p
{

struct line_edit
{
    enum class refresh_flag
    {
        Clean = 1 << 0, ///< Clean the old prompt from the screen.
        Write = 1 << 1, ///< Rewrite the prompt on the screen.
        All   = Clean | Write ///< Do both.
    };

    using refresh_flags = tim::flags<refresh_flag>;

    enum class history_dir
    {
        Prev,
        Next
    };

    tim::a_telnet_service *_telnet = nullptr;

    tim::line_edit::completer_fn _completer;
    tim::line_edit::hinter_fn _hinter;

    bool _in_completion = false; /* The user pressed `Tab` and we are now in completion
                                  * mode, so input is handled by complete_line(). */
    std::size_t _completion_idx = 0; /* Index of next completion to propose. */
    std::wstring _line; /* Edited line buffer. */
    static constexpr const std::size_t MAX_LINE_SIZE = 4096;
    std::wstring _prompt; /* Prompt to display. */
    std::size_t _plen = 0; /* Prompt length. We calculate uncolorized prompt length here. */
    std::size_t _pos = 0; /* Current cursor position. */
    std::size_t _old_pos = 0; /* Previous refresh cursor position. */
    std::size_t _cols = 0; /* Number of columns in terminal. */
    std::size_t _old_rows = 0; /* Rows used by last refreshed line (multiline mode) */

    using history = std::vector<std::wstring>;
    history _history;
    int _history_idx = 0; /* The history index we are currently editing. */

    bool _mask_mode = false; /* Show "***" instead of input. For passwords. */
    bool _ml_mode = false; /* Multi line mode. Default is single line. */

    std::size_t _line_count = 0; // We need this just to omit `\r\n` when we first call new_line().

    void history_add(const std::wstring &line);

    void beep();

    void refresh_line_with_completion(const tim::line_edit::completions *c, refresh_flags flags);
    std::int32_t complete_line(std::int32_t key_pressed);

    void refresh_show_hints(std::wstring &s);
    void refresh_single_line(refresh_flags flags);
    void refresh_multi_line(refresh_flags flags);
    void refresh_line_with_flags(refresh_flags flags);
    void refresh_line();

    bool edit_insert(std::int32_t c);
    void edit_move_left();
    void edit_move_right();
    void edit_move_home();
    void edit_move_end();
    void edit_delete();
    void edit_backspace();
    void edit_delete_prev_word();
    void edit_history_next(history_dir dir);
};

}

}
