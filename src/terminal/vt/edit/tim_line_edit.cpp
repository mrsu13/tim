#include "tim_line_edit.h"

#include "tim_line_edit_p.h"

#include "tim_a_protocol.h"
#include "tim_file_tools.h"
#include "tim_string_tools.h"
#include "tim_vt.h"

#include "utf8/utf8.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>


/**
 * \class tim::line_edit
 *
 * \brief Terminal line editor. Auto-completing is supported.
 *
 * This implementation is based on <https://github.com/antirez/linenoise.git>.
 */

// Public

/**
 * Constructor.
 */
tim::line_edit::line_edit(tim::vt *term)
    : _d(new tim::p::line_edit())
{
    assert(term);

    _d->_terminal = term;
    _d->_cols = _d->_terminal->cols();
}

tim::line_edit::~line_edit() = default;

tim::vt *tim::line_edit::terminal() const
{
    return _d->_terminal;
}

std::string tim::line_edit::prompt() const
{
    return tim::from_wstring(_d->_prompt);
}

/**
 * Set prompt.
 */
void tim::line_edit::set_prompt(const std::string &prompt)
{
    _d->_prompt = tim::to_wstring(prompt);
    _d->_plen = tim::vt::strlen(prompt);
}

bool tim::line_edit::empty() const
{
    return _d->_line.empty();
}

/**
 * Use this method when get_line() returns status::Finished.
 *
 * \return Edited line.
 */
std::string tim::line_edit::line() const
{
    return tim::from_wstring(_d->_line);
}

/**
 * Start line editing. It will:
 *
 * 1. Show the prompt.
 * 2. Return control to the user, that will have to call get_line()
 *    each time there is some data coming from the input stream.
 *
 * Here is how you call the function. You call tim_ledit_new_line(), then you
 * call get_line() until it returns \c status::Finished,
 * \c status::Exit or \c status::Error.
 *
 * Between get_line() calls you may call hide() and
 * show() if you want to show some input coming asynchronously,
 * without mixing it with the currently edited line.
 *
 * \return \c true in the case of success, and \c false if the output
 * writing failed.
 *
 * \sa tim_ledit_get_line()
 */
bool tim::line_edit::new_line()
{
    _d->_in_completion = false;
    _d->_old_pos = _d->_pos = 0;
    _d->_cols = _d->_terminal->cols();
    _d->_old_rows = 0;
    _d->_history_idx = 0;

    _d->_line.clear();

   /* The latest history entry is always our current buffer, that
    * initially is just an empty string. */
    _d->_history.emplace_back(L"");

    const std::string p = prompt();
    return (!_d->_line_count++
                        || _d->_terminal->protocol()->write("\n", 1))
                    && _d->_terminal->protocol()->write(p.c_str(), p.size());
}

/**
 * Call this function to process user input when there are data in the input stream.
 *
 * \return Editing status to check if the line editing is finished.
 *
 * \sa new_line()
 */
tim::line_edit::status tim::line_edit::get_line(const char *data, std::size_t size)
{
    assert(data);

    if (!size)
        return status::Continue;

    std::int32_t c;
    {
        const utf8_int8_t *next = utf8codepoint((const utf8_int8_t *)data, &c);
        const int d = next - (const utf8_int8_t *)data;
        data += d;
        size -= d;
    }

    if ((_d->_in_completion
                || c == (char)tim::key::Tab)
            && _d->_completer)
    {
        c = _d->complete_line(c);
        if (c < 0)
            return status::Error;
        if (c == 0)
            return status::Continue;
    }

    char seq[3];

    switch (c)
    {
        case (char)tim::key::Enter:
        case (char)tim::key::Cr:
            if (!_d->_history.empty())
                _d->_history.pop_back();
            if (_d->_ml_mode)
                _d->edit_move_end();
            if (_d->_hinter)
            {
                /* Force a refresh without hints to leave the previous
                 * line as the user typed it after a newline. */
                hinter_fn hc = _d->_hinter;
                _d->_hinter = nullptr;
                _d->refresh_line();
                _d->_hinter = hc;
            }
            _d->history_add(_d->_line);
            return status::Finished;

        case (char)tim::key::Ctrl_C:
            _d->_terminal->protocol()->write_str("^C");
            clear();
            return status::Finished;

        case (char)tim::key::Backspace:
        case (char)tim::key::Ctrl_H:
            _d->edit_backspace();
            break;

        case (char)tim::key::Ctrl_D: /* Remove char at right of cursor, or if the
                                       line is empty, act as end-of-file. */
            if (!_d->_line.empty())
                _d->edit_delete();
            else
            {
                if (!_d->_history.empty())
                    _d->_history.pop_back();
                return status::Exit;
            }
            break;

        case (char)tim::key::Ctrl_T: /* Swaps current character with previous. */
            if (_d->_pos > 0
                    && _d->_pos < _d->_line.size())
            {
                int aux = _d->_line[_d->_pos - 1];
                _d->_line[_d->_pos - 1] = _d->_line[_d->_pos];
                _d->_line[_d->_pos] = aux;
                if (_d->_pos != _d->_line.size() - 1)
                    ++_d->_pos;
                _d->refresh_line();
            }
            break;

        case (char)tim::key::Ctrl_B:
            _d->edit_move_left();
            break;

        case (char)tim::key::Ctrl_F:
            _d->edit_move_right();
            break;

        case (char)tim::key::Ctrl_P:
            _d->edit_history_next(tim::p::line_edit::history_dir::Prev);
            break;

        case (char)tim::key::Ctrl_N:
            _d->edit_history_next(tim::p::line_edit::history_dir::Next);
            break;

        case (char)tim::key::Esc: /* Escape sequence */
            /* Read the next two bytes representing the escape sequence.
             * Use two calls to handle slow terminals returning the two
             * chars at different times. */
            if (size < 2)
                break;
            seq[0] = *data++;
            seq[1] = *data++;
            size -= 2;

            /* ESC [ sequences. */
            if (seq[0] == '[')
            {
                if (seq[1] >= '0'
                        && seq[1] <= '9')
                {
                    /* Extended escape, read additional byte. */
                    if (!size)
                        break;
                    seq[2] = *data++;
                    --size;
                    if (seq[2] == '~')
                    {
                        switch (seq[1])
                        {
                            case '3': /* Delete key. */
                                _d->edit_delete();
                                break;
                        }
                    }
                }
                else
                {
                    switch (seq[1])
                    {
                        case 'A': /* Up */
                            _d->edit_history_next(tim::p::line_edit::history_dir::Prev);
                            break;
                        case 'B': /* Down */
                            _d->edit_history_next(tim::p::line_edit::history_dir::Next);
                            break;
                        case 'C': /* Right */
                            _d->edit_move_right();
                            break;
                        case 'D': /* Left */
                            _d->edit_move_left();
                            break;
                        case 'H': /* Home */
                            _d->edit_move_home();
                            break;
                        case 'F': /* End*/
                            _d->edit_move_end();
                            break;
                    }
                }
            }

            /* ESC O sequences. */
            else if (seq[0] == 'O')
            {
                switch (seq[1])
                {
                    case 'H': /* Home */
                        _d->edit_move_home();
                        break;
                    case 'F': /* End */
                        _d->edit_move_end();
                        break;
                }
            }
            break;

        case (char)tim::key::Ctrl_U: /* Delete the whole line. */
            _d->_line.clear();
            _d->_pos = 0;
            _d->refresh_line();
            break;

        case (char)tim::key::Ctrl_K: /* Delete from current to end of line. */
            _d->_line.erase(_d->_line.cbegin() + _d->_pos, _d->_line.cend());
            _d->refresh_line();
            break;

        case (char)tim::key::Ctrl_A: /* Go to the start of the line. */
            _d->edit_move_home();
            break;

        case (char)tim::key::Ctrl_E: /* Go to the end of the line. */
            _d->edit_move_end();
            break;

        case (char)tim::key::Ctrl_L: /* Clear screen. */
            _d->_terminal->clear();
            _d->refresh_line();
            break;

        case (char)tim::key::Ctrl_W: /* Delete previous word. */
            _d->edit_delete_prev_word();
            break;

        default:
            _d->edit_insert(c);
            break;
    }

    return status::Continue;
}

void tim::line_edit::clear()
{
    _d->_line.clear();
    _d->_in_completion = true;
    _d->_mask_mode = false;
}

/**
 * Hide the line being edited to output outgoing data asynchronously.
 *
 * \sa show() new_line()
 */

void tim::line_edit::hide()
{
    if (_d->_ml_mode)
        _d->refresh_multi_line(tim::p::line_edit::refresh_flag::Clean);
    else
        _d->refresh_single_line(tim::p::line_edit::refresh_flag::Clean);
}

/**
 * Show the line being edited after tim_ledit_hide().
 *
 * \sa hide() new_line()
 */

void tim::line_edit::show()
{
    if (_d->_in_completion)
        _d->refresh_line_with_completion(nullptr, tim::p::line_edit::refresh_flag::Write);
    else
        _d->refresh_line_with_flags(tim::p::line_edit::refresh_flag::Write);
}

/**
 * Enable/disable multi-line editing mode (disabled by default).
*/
void tim::line_edit::set_multi_line(bool enable)
{
    _d->_ml_mode = enable;
}

/**
 * Enable/disabled "mask mode". When it is enabled, instead of the input that
 * the user is typing, the terminal will just display a corresponding
 * number of asterisks, like "****". This is useful for passwords and other
 * secrets that should not be displayed.
 */
void tim::line_edit::set_mask_mode(bool enable)
{
    _d->_mask_mode = enable;
}

/**
 * Store the history to a file.
 *
 * \return \c true if succeeded, and \c false otherwise.
 */
bool tim::line_edit::history_save(const std::filesystem::path &path) const
{
    assert(!path.empty() && "History file path must not be empty.");

    const mode_t old_umask = umask(S_IXUSR | S_IRWXG | S_IRWXO);

    std::FILE *fp = std::fopen(path.string().c_str(), "w");
    if (!fp)
    {
        TIM_TRACE(Error,
                  TIM_TR("Failed to open History file '%s' for writing: %s"_en,
                         "Ошибка при открытии файла истории '%s' на запись: %s"_ru),
                  path.string().c_str(), std::strerror(errno));
        return false;
    }
    umask(old_umask);
    chmod(path.string().c_str(), S_IRUSR | S_IWUSR);
    for (const std::wstring &s: _d->_history)
        if (!s.empty()
                && std::fprintf(fp, "%s\n", tim::from_wstring(s).c_str()) < 0)
        {
            TIM_TRACE(Error,
                      TIM_TR("Failed to write to History file '%s': %s"_en,
                             "Ошибка записи в файл истории '%s': %s"_ru),
                      path.string().c_str(), std::strerror(errno));
            break;
        }
    std::fclose(fp);

    return true;
}

/**
 * Load the history from a file.
 *
 * \return \c true if succeeded, and \c false otherwise.
 */
bool tim::line_edit::history_load(const std::filesystem::path &path)
{
    assert(!path.empty() && "History file path must not be empty.");

    if (!std::filesystem::exists(path))
        return false;

    std::FILE *fp = std::fopen(path.string().c_str(), "r");
    if (!fp)
    {
        TIM_TRACE(Error,
                  TIM_TR("Failed to open History file '%s' for reading: %s"_en,
                         "Ошибка при открытии файла истории '%s' на чтение: %s"_ru),
                  path.string().c_str(), std::strerror(errno));
        return false;
    }

    std::vector<char> buf(_d->MAX_LINE_SIZE);

    while (std::fgets(&buf[0], buf.size(), fp))
    {
        char *p = std::strchr(&buf[0], '\r');
        if (!p)
            p = std::strchr(&buf[0], '\n');
        if (p)
            *p = '\0';
        _d->history_add(tim::to_wstring(&buf[0]));
    }
    std::fclose(fp);

    return true;
}

/**
 * Set completing callback.
 */
void tim::line_edit::set_completer(tim::line_edit::completer_fn fn)
{
    _d->_completer = fn;
}

/**
 * Set hinting callback.
 */
void tim::line_edit::set_hinter(tim::line_edit::hinter_fn fn)
{
    _d->_hinter = fn;
}

// Private

/**
 * Add line to the history.
 */
void tim::p::line_edit::history_add(const std::wstring &line)
{
    if (!line.empty())
        _history.emplace_back(line);
}

/** Beep, used for completion when there is nothing to complete or when all
  * the choices were already shown. This method does nothing.
  */
void tim::p::line_edit::beep()
{
}

/* Called by complete_line() and show() to render the current
 * edited line with the proposed completion. If the current completion table
 * is already available, it is passed as second argument, otherwise the
 * function will use the callback to obtain it.
 *
 * Flags are the same as refresh_line*(). */
void tim::p::line_edit::refresh_line_with_completion(const tim::line_edit::completions *c, refresh_flags flags)
{
    assert(_completer);

    /* Obtain the completions if the caller didn't provide one. */
    const tim::line_edit::completions *lc =
            !c
                    || c->empty()
                ? new tim::line_edit::completions(_completer(tim::from_wstring(_line)))
                : c;

    /* Show the edited line with completion if possible, or just refresh. */
    if (_completion_idx < lc->size())
    {
        const std::size_t p = _pos;
        const std::wstring l = _line;
        _pos = lc->at(_completion_idx).size();
        _line = tim::to_wstring(lc->at(_completion_idx));
        refresh_line_with_flags(flags);
        _pos = p;
        _line = l;
    }
    else
        refresh_line_with_flags(flags);

    if (c != lc)
        delete lc;
}

/* This is an helper function for edit(), and is called when the
 * user types the [Tab] key in order to complete the string currently in the
 * input.
 *
 * If the function returns non-zero, the caller should handle the
 * returned value as a byte read from the standard input, and process
 * it as usually: this basically means that the function may return a byte
 * read from the terminal but not processed. Otherwise, if zero is returned,
 * the input was consumed by the complete_line() function to navigate the
 * possible completions, and the caller should read for the next characters
 * from the input stream. */
std::int32_t tim::p::line_edit::complete_line(std::int32_t key_pressed)
{
    assert(_completer);

    char c = key_pressed;

    tim::line_edit::completions lc = _completer(tim::from_wstring(_line));
    if (lc.empty())
    {
        beep();
        _in_completion = false;
        c = 0; // Never allow tabs.
    }
    else
    {
        switch (c)
        {
            case (char)tim::key::Tab:
                if (!_in_completion)
                {
                    _in_completion = true;
                    _completion_idx = 0;
                }
                else
                {
                    _completion_idx = (_completion_idx + 1) % (lc.size() + 1);
                    if (_completion_idx == lc.size())
                        beep();
                }
                c = 0;
                break;

            case (char)tim::key::Esc:
                /* Re-show original buffer. */
                if (_completion_idx < lc.size())
                    refresh_line();
                _in_completion = false;
                c = 0;
                break;

            default:
                /* Update buffer and return. */
                if (_completion_idx < lc.size())
                {
                    _line = tim::to_wstring(lc.at(_completion_idx));
                    _pos = _line.size();
                }
                _in_completion = false;
                break;
        }

        /* Show completion or original buffer. */
        if (_in_completion
                && _completion_idx < lc.size())
            refresh_line_with_completion(&lc, refresh_flag::All);
        else
            refresh_line();
    }

    return c; /* Return last read character */
}

/** Helper of refresh_single_line() and refresh_multi_line() to show hints
  * to the right of the prompt.
  */
void tim::p::line_edit::refresh_show_hints(std::wstring &s)
{
    if (!_hinter)
        return;

    _cols = _terminal->cols();

    if (_plen + _line.size() < _cols)
    {
        int color = -1, bold = 0;
        const std::wstring hint = tim::to_wstring(_hinter(tim::from_wstring(_line), color, bold));
        if (!hint.empty())
        {
            if (bold == 1
                    && color == -1)
                color = 37;
            if (color != -1
                    || bold != 0)
                s += tim::to_wstring(tim::sprintf("\033[%d;%d;49m", bold, color));
            s += hint;
            if (color != -1
                    || bold != 0)
                s += L"\033[0m";
        }
    }
}

/** Single line low level line refresh.
  *
  * Rewrite the currently edited line accordingly to the buffer content,
  * cursor position, and number of columns of the terminal.
  *
  * The function can just remove the old prompt, just write it, or both.
  */
void tim::p::line_edit::refresh_single_line(refresh_flags flags)
{
    _cols = _terminal->cols();

    const wchar_t *buf = _line.c_str();
    std::size_t len = _line.size();
    std::size_t pos = _pos;

    while (_plen + pos >= _cols)
    {
        ++buf;
        --len;
        --pos;
    }

    while (_plen + len > _cols)
        --len;

    /* Cursor to the left edge. */
    std::wstring ws(L"\r");

    if (flags.test(refresh_flag::Write))
    {
        /* Write the prompt and the current buffer content. */
        ws += _prompt;
        if (_mask_mode)
            ws += std::wstring(_line.size(), L'*');
        else
            ws += std::wstring_view(buf, len);
        /* Show hits if any. */
        refresh_show_hints(ws);
    }

    /* Erase to right. */
    ws += L"\x1b[0K";

    if (flags.test(refresh_flag::Write))
    {
        /* Move cursor to the original position. */
        ws += tim::to_wstring(tim::sprintf("\r\x1b[%dC", (int)(pos + _plen)));
    }

    const std::string s = tim::from_wstring(ws);
    if (!_terminal->protocol()->write(s.c_str(), s.size()))
    {
        /* Can't recover from write error. */
    }
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 *
 * The function can just remove the old prompt, just write it, or both.
 */
void tim::p::line_edit::refresh_multi_line(refresh_flags flags)
{
    _cols = _terminal->cols();

    int rows = static_cast<int>((_plen + _line.size() + _cols - 1) / _cols); /* Rows used by current buf. */
    int rpos = static_cast<int>((_plen + _old_pos + _cols) / _cols); /* Cursor relative row. */
    int rpos2; /* rpos after refresh. */
    int col; /* Column position, zero-based. */
    int old_rows = static_cast<int>(_old_rows);
    int j;

    _old_rows = rows;

    /* First step: clear all the lines used before. To do so start by
     * going to the last row. */

    std::wstring ws;

    if (flags.test(refresh_flag::Clean))
    {
        if (old_rows - rpos > 0)
            ws += tim::to_wstring(tim::sprintf("\x1b[%dB", old_rows - rpos));

        /* Now for every row clear it, go up. */
        for (j = 0; j < old_rows - 1; ++j)
            ws += L"\r\x1b[0K\x1b[1A";
    }

    if (flags.test(refresh_flag::All))
    {
        /* Clean the top line. */
        ws += L"\r\x1b[0K";
    }

    if (flags.test(refresh_flag::Write))
    {
        /* Write the prompt and the current buffer content. */
        ws += _prompt;
        if (_mask_mode)
            ws += std::wstring(_line.size(), L'*');
        else
            ws += _line;

        /* Show hits if any. */
        refresh_show_hints(ws);

        /* If we are at the very end of the screen with our prompt, we need to
         * emit a newline and move the prompt to the first column. */
        if (_pos
                && _pos == _line.size()
                && (_pos + _plen) % _cols == 0)
        {
            ws += L"\n\r";
            ++rows;
            if (rows > static_cast<int>(_old_rows))
                _old_rows = rows;
        }

        /* Move cursor to right position. */
        rpos2 = static_cast<int>((_plen + _pos + _cols) / _cols); /* Current cursor relative row. */

        /* Go up till we reach the expected position. */
        if (rows - rpos2 > 0)
            ws += tim::to_wstring(tim::sprintf("\x1b[%dA", rows - rpos2));

        /* Set column. */
        col = (_plen + (int)_pos) % (int)_cols;
        if (col)
            ws += tim::to_wstring(tim::sprintf("\r\x1b[%dC", col));
        else
            ws += L'\r';
    }

    _old_pos = _pos;

    const std::string s = tim::from_wstring(ws);
    if (!_terminal->protocol()->write(s.c_str(), s.size()))
    {
        /* Can't recover from write error. */
    }
}

/** Calls the two low level functions refresh_single_line() or
  * refresh_multi_line() according to the selected mode.
  */
void tim::p::line_edit::refresh_line_with_flags(refresh_flags flags)
{
    if (_ml_mode)
        refresh_multi_line(flags);
    else
        refresh_single_line(flags);
}

/** Utility function to avoid specifying refresh_flagg::All all the times.
  */
void tim::p::line_edit::refresh_line()
{
    refresh_line_with_flags(refresh_flag::All);
}

/** Insert the character \a c at cursor current position.
 *
 * On error writing to the terminal \c false is returned, otherwise \c true.
 */
bool tim::p::line_edit::edit_insert(std::int32_t c)
{
    _cols = _terminal->cols();

    _line.insert(_pos, 1, c);
    ++_pos;
    if (_pos == _line.size()
            && !_ml_mode
            && _plen + _line.size() < _cols
            && !_hinter)
    {
        /* Avoid a full update of the line in the trivial case. */
        const std::int32_t d = _mask_mode
                            ? '*'
                            : c;
        std::string s(utf8codepointsize(d), 0);
        utf8catcodepoint((utf8_int8_t *)(&s[0]), d, s.size());
        if (!_terminal->protocol()->write(s.c_str(), s.size()))
            return false;
    }
    else
        refresh_line();
    return true;
}

/** Move cursor on the left.
 */
void tim::p::line_edit::edit_move_left()
{
    if (_pos > 0)
    {
        --_pos;
        refresh_line();
    }
}

/** Move cursor on the right.
 */
void tim::p::line_edit::edit_move_right()
{
    if (_pos != _line.size())
    {
        ++_pos;
        refresh_line();
    }
}

/** Move cursor to the start of the line.
 */
void tim::p::line_edit::edit_move_home()
{
    if (_pos != 0)
    {
        _pos = 0;
        refresh_line();
    }
}

/** Move cursor to the end of the line.
 */
void tim::p::line_edit::edit_move_end()
{
    if (_pos != _line.size())
    {
        _pos = _line.size();
        refresh_line();
    }
}

/** Substitute the currently edited line with the next or previous history
 * entry as specified by \a dir.
 */
void tim::p::line_edit::edit_history_next(history_dir dir)
{
    if (_history.size() > 1)
    {
        /* Update the current history entry before to
         * overwrite it with the next one. */
        _history[_history.size() - 1 - _history_idx] = _line;
        /* Show the new entry. */
        _history_idx +=
            dir == history_dir::Prev
                ? 1
                : -1;
        if (_history_idx < 0)
        {
            _history_idx = 0;
            return;
        }
        if (_history_idx >= (int)_history.size())
        {
            _history_idx = (int)_history.size() - 1;
            return;
        }

        _line = _history[_history.size() - 1 - _history_idx];
        _pos = _line.size();
        refresh_line();
    }
}

/** Delete the character at the right of the cursor without altering the cursor
  * position. Basically this is what happens with the [Delete] keyboard key.
  */
void tim::p::line_edit::edit_delete()
{
    if (!_line.empty()
            && _pos < _line.size())
    {
        _line.erase(_pos, 1);
        refresh_line();
    }
}

/** Backspace implementation.
 */
void tim::p::line_edit::edit_backspace()
{
    if (!_line.empty()
            && _pos > 0)
    {
        _line.erase(_pos - 1, 1);
        --_pos;
        refresh_line();
    }
}

/** Delete the previous word, maintaining the cursor at the start of the
  * current word.
  */
void tim::p::line_edit::edit_delete_prev_word()
{
    const std::size_t old_pos = _pos;

    while (_pos > 0
                && std::isspace(_line[_pos - 1]))
        --_pos;
    while (_pos > 0
                && !std::isspace(_line[_pos - 1]))
        --_pos;

    _line.erase(_line.cbegin() + _pos, _line.cbegin() + old_pos);
    refresh_line();
}
