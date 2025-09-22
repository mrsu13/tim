#include "tim_line_edit.h"

#include "tim_file.h"
#include "tim_trace.h"
#include "tim_vt.h"
#include "tim_wstr.h"
#include "tim_wstring.h"

#include "tim_line_edit_p.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <wchar.h>


/**
 * \brief
 * This implementation is based on <https://github.com/antirez/linenoise.git>.
 */

typedef enum tim_key : char
{
	TimKeyNull      =   0,
	TimKeyCtrlA     =   1,
	TimKeyCtrlB     =   2,
	TimKeyCtrlC     =   3,
	TimKeyCtrlD     =   4,
	TimKeyCtrlE     =   5,
	TimKeyCtrlF     =   6,
	TimKeyCtrlH     =   8,
	TimKeyTab       =   9,
    TimKeyCr        =  10,
	TimKeyCtrlK     =  11,
	TimKeyCtrlL     =  12,
	TimKeyEnter     =  13,
	TimKeyCtrlN     =  14,
	TimKeyCtrlP     =  16,
	TimKeyCtrlT     =  20,
	TimKeyCtrlU     =  21,
	TimKeyCtrlW     =  23,
	TimKeyEsc       =  27,
	TimKeyBackspace = 127
} tim_key_t;

typedef enum tim_line_edit_refresh_flag
{
    TimRefreshClean = 1 << 0, ///< Clean the old prompt from the screen.
    TimRefreshWrite = 1 << 1, ///< Rewrite the prompt on the screen.
    TimRefreshAll   = TimRefreshClean | TimRefreshWrite ///< Do both.
} tim_line_edit_refresh_flag_t;

typedef enum tim_line_edit_history_dir
{
    TimLineEditHistoryPrev,
    TimLineEditHistoryNext
} tim_line_edit_history_dir_t;

static const size_t TIM_BUFFER_SIZE = 4096;
static const size_t TIM_HISTORY_SIZE = 1000;

static void tim_line_edit_beep(tim_line_edit_t *le);

static void tim_line_edit_refresh_line_with_completion(tim_line_edit_t *le,
                                                       const tim_line_edit_completions_t *c,
                                                       unsigned refresh_flags);
static char tim_line_edit_complete_line(tim_line_edit_t *le, int key_pressed);

static void tim_line_edit_refresh_show_hints(tim_line_edit_t *le, tim_wstr_t *s);
static void tim_line_edit_refresh_single_line(tim_line_edit_t *le, unsigned refresh_flags);
static void tim_line_edit_refresh_multi_line(tim_line_edit_t *le, unsigned refresh_flags);
static void tim_line_edit_refresh_line_with_flags(tim_line_edit_t *le, unsigned refresh_flags);
static void tim_line_edit_refresh_line(tim_line_edit_t *le);

static bool tim_line_edit_insert(tim_line_edit_t *le, char c);
static void tim_line_edit_move_left(tim_line_edit_t *le);
static void tim_line_edit_move_right(tim_line_edit_t *le);
static void tim_line_edit_move_home(tim_line_edit_t *le);
static void tim_line_edit_move_end(tim_line_edit_t *le);
static void tim_line_edit_delete(tim_line_edit_t *le);
static void tim_line_edit_backspace(tim_line_edit_t *le);
static void tim_line_edit_delete_prev_word(tim_line_edit_t *le);

static void tim_line_edit_history_add(tim_line_edit_t *le, const wchar_t *line);
static void tim_line_edit_history_next(tim_line_edit_t *le, tim_line_edit_history_dir_t dir);

/**
 * \file tim_line_edit.h
 * \brief An abstract class implementing terminal text line edit.
 * Autocompleting is supported.
 */

// Public

void tim_line_edit_init(tim_line_edit_t *le, tim_line_edit_write_t write, void *user_data)
{
    assert(le);
    assert(write);

    memset(le, 0, sizeof(*le));

    le->write = write;
    le->user_data = user_data;

    le->buf_size = TIM_BUFFER_SIZE;
    le->buf = (wchar_t *)calloc(1, le->buf_size);
    assert(le->buf && "Failed to allocate memory for line editor's buffer.");
    le->cols = 80;
    le->history_max_size = TIM_HISTORY_SIZE;
}

void tim_line_edit_destroy(tim_line_edit_t *le)
{
    assert(le);

    if (le->history)
    {
        for (int i = 0; i < le->history_len; ++i)
            free(le->history[i]);
        free(le->history);
    }

    free(le->prompt);

    memset(le, 0, sizeof(*le));
}

tim_line_edit_t *tim_line_edit_new(tim_line_edit_write_t write, void *user_data)
{
    tim_line_edit_t *le = (tim_line_edit_t *)malloc(sizeof(tim_line_edit_t));
    assert(le && "Failed to allocate memory for the line editor.");

    tim_line_edit_init(le, write, user_data);

    return le;
}

void tim_line_edit_free(tim_line_edit_t *le)
{
    tim_line_edit_destroy(le);
    free(le);
}

/**
 * Start line editing. It will:
 *
 * 1. Show the prompt.
 * 2. Return control to the user, that will have to call tim_line_edit_get_line()
 *    each time there is some data coming from the input stream.
 *
 * Here is how you call the function. You call tim_line_edit_new_line(), then you
 * call tim_line_edit_get_line() until it returns \c TimLineEditFinished,
 * \c TimLineEditExit or \c TimLineEditError.
 *
 * Between tim_line_edit_get_line() calls you may call tim_line_edit_hide() and
 * tim_line_edit_show() if you want to show some input coming asynchronously,
 * without mixing it with the currently edited line.
 *
 * \return \c true in the case of success, and \c false if the output
 * writing failed.
 *
 * \see tim_line_edit_get_line()
 */
bool tim_line_edit_new_line(tim_line_edit_t *le, const char *prompt)
{
    assert(le);
    assert(prompt);

    le->in_completion = false;

    free(le->prompt);
    tim_to_ws(&le->prompt, prompt);
    le->plen = tim_vt_strlen(prompt);
    le->psize = wcslen(le->prompt);

    le->old_pos = le->pos = 0;
    le->len = 0;
    // // FIXME! le->cols = ?;
    le->old_rows = 0;
    le->history_idx = 0;

    /* Buffer starts empty. */
    le->buf[0] = L'\0';
    --(le->buf_size); /* Make sure there is always space for the nulterm. */

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    tim_line_edit_history_add(le, L"");

    return le->write(le->user_data, "\r\n", 2) > 0
                && le->write(le->user_data, prompt, le->psize) > 0;
}

/**
 * Call this function to process user input when there are data in the input stream.
 *
 * \return Editing status to check if the line editing is finished.
 *
 * \sa tim_line_edit_new_line()
 */
tim_line_edit_status_t tim_line_edit_get_line(tim_line_edit_t *le, const char *data, size_t size)
{
    assert(le);
    assert(data);

    if (!size)
        return TimLineEditContinue;

    char c = *data++;
    --size;

    if ((le->in_completion
                || c == TimKeyTab)
            && le->complete)
    {
        c = tim_line_edit_complete_line(le, c);
        if (c < 0)
            return TimLineEditError;
        if (c == 0)
            return TimLineEditContinue;
    }

    char seq[3];

    switch (c)
    {
        case TimKeyEnter:
        case TimKeyCr:
            --(le->history_len);
            free(le->history[le->history_len]);
            if (le->ml_mode)
                tim_line_edit_move_end(le);
            if (le->hint)
            {
                /* Force a refresh without hints to leave the previous
                 * line as the user typed it after a newline. */
                tim_line_edit_hinter_t *hc = le->hint;
                le->hint = NULL;
                tim_line_edit_refresh_line(le);
                le->hint = hc;
            }
            tim_line_edit_history_add(le, le->buf);
            return TimLineEditFinished;

        case TimKeyCtrlC:
            return TimLineEditExit;

        case TimKeyBackspace:
        case TimKeyCtrlH:
            tim_line_edit_backspace(le);
            break;

        case TimKeyCtrlD: /* Remove char at right of cursor, or if the
                             line is empty, act as end-of-file. */
            if (le->buf
                    && *(le->buf))
                tim_line_edit_delete(le);
            else
            {
                --(le->history_len);
                free(le->history[le->history_len]);
                return TimLineEditExit;
            }
            break;

        case TimKeyCtrlT: /* Swaps current character with previous. */
            if (le->pos > 0
                    && le->pos < le->len)
            {
                const wchar_t aux = le->buf[le->pos - 1];
                le->buf[le->pos - 1] = le->buf[le->pos];
                le->buf[le->pos] = aux;
                if (le->pos != le->len - 1)
                    ++le->pos;
                tim_line_edit_refresh_line(le);
            }
            break;

        case TimKeyCtrlB:
            tim_line_edit_move_left(le);
            break;

        case TimKeyCtrlF:
            tim_line_edit_move_right(le);
            break;

        case TimKeyCtrlP:
            tim_line_edit_history_next(le, TimLineEditHistoryPrev);
            break;

        case TimKeyCtrlN:
            tim_line_edit_history_next(le, TimLineEditHistoryNext);
            break;

        case TimKeyEsc: /* Escape sequence */
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
                                tim_line_edit_delete(le);
                                break;
                        }
                    }
                }
                else
                {
                    switch (seq[1])
                    {
                        case 'A': /* Up */
                            tim_line_edit_history_next(le, TimLineEditHistoryPrev);
                            break;
                        case 'B': /* Down */
                            tim_line_edit_history_next(le, TimLineEditHistoryNext);
                            break;
                        case 'C': /* Right */
                            tim_line_edit_move_right(le);
                            break;
                        case 'D': /* Left */
                            tim_line_edit_move_left(le);
                            break;
                        case 'H': /* Home */
                            tim_line_edit_move_home(le);
                            break;
                        case 'F': /* End*/
                            tim_line_edit_move_end(le);
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
                        tim_line_edit_move_home(le);
                        break;
                    case 'F': /* End */
                        tim_line_edit_move_end(le);
                        break;
                }
            }
            break;

        case TimKeyCtrlU: /* Delete the whole line. */
            le->buf[0] = L'\0';
            le->pos = le->len = 0;
            tim_line_edit_refresh_line(le);
            break;

        case TimKeyCtrlK: /* Delete from current to end of line. */
            le->buf[le->pos] = L'\0';
            le->len = le->pos;
            tim_line_edit_refresh_line(le);
            break;

        case TimKeyCtrlA: /* Go to the start of the line. */
            tim_line_edit_move_home(le);
            break;

        case TimKeyCtrlE: /* Go to the end of the line. */
            tim_line_edit_move_end(le);
            break;

        case TimKeyCtrlL: /* Clear screen. */
            tim_line_edit_clear_screen(le);
            tim_line_edit_refresh_line(le);
            break;

        case TimKeyCtrlW: /* Delete previous word. */
            tim_line_edit_delete_prev_word(le);
            break;

        default:
            tim_line_edit_insert(le, c);
            break;
    }

    return TimLineEditContinue;
}

bool tim_line_edit_empty(const tim_line_edit_t *le)
{
    assert(le);

    return *le->buf;
}

char *tim_line_edit_line(const tim_line_edit_t *le)
{
    assert(le);

    char *line;
    tim_from_ws(&line, le->buf);
    return line;
}

/**
 * Hide the line being edited to output outgoing data asynchronously.
 *
 * \sa tim_line_edit_show() tim_line_edit_new_line()
 */
void tim_line_edit_hide(tim_line_edit_t *le)
{
    assert(le);

    if (le->ml_mode)
        tim_line_edit_refresh_multi_line(le, TimRefreshClean);
    else
        tim_line_edit_refresh_single_line(le, TimRefreshClean);
}

/**
 * Show the line being edited after tim_line_edit_hide().
 *
 * \sa tim_line_edit_hide() tim_line_edit_new_line()
 */
void tim_line_edit_show(tim_line_edit_t *le)
{
    assert(le);

    if (le->in_completion)
        tim_line_edit_refresh_line_with_completion(le, NULL, TimRefreshWrite);
    else
        tim_line_edit_refresh_line_with_flags(le, TimRefreshWrite);
}

/**
 * Задать ширину терминала.
 *
 * \param cols Количество столбцов. Минимум --- \c 16. Если переданное значение
 * меньше, будет использована ширина терминала в \c 16 столбцов.
 */
void tim_line_edit_set_term_width(tim_line_edit_t *le, size_t cols)
{
    assert(le);

    le->cols = cols > 16
                    ? cols
                    : 16;
}

/**
 * Enable/disable multiline editing mode (disabled by default).
*/
void tim_line_edit_set_multiline(tim_line_edit_t *le, bool enable)
{
    assert(le);

    le->ml_mode = enable;
}

/** Enable/disabled "mask mode". When it is enabled, instead of the input that
 * the user is typing, the terminal will just display a corresponding
 * number of asterisks, like "****". This is useful for passwords and other
 * secrets that should not be displayed.
 */
void tim_line_edit_set_mask_mode(tim_line_edit_t *le, bool enable)
{
    assert(le);

    le->mask_mode = enable;
}


/**
 * Clear screen.
 */
void tim_line_edit_clear_screen(tim_line_edit_t *le)
{
    assert(le);

    static const char CMD[] = "\x1b[H\x1b[2J";
    le->write(le->user_data, CMD, sizeof(CMD));
}


/* Set the maximum length for the history. This function can be called even
 * if there is already some history, the function will make sure to retain
 * just the latest 'len' elements if the new history length value is smaller
 * than the amount of items already inside the history.
 */
void tim_line_edit_history_set_max_size(tim_line_edit_t *le, size_t size)
{
    assert(le);
    assert(size);

    if (le->history)
    {
        int to_copy = le->history_len;

        wchar_t **new_history = malloc(sizeof(wchar_t *) * size);
        assert(new_history && "Failed to allocate memory for History.");

        /* If we can't copy everything, free the elements we'll not use. */
        if (size < to_copy)
        {
            for (int i = 0; i < to_copy - size; ++i)
                free(le->history[i]);
            to_copy = size;
        }
        wmemset((wchar_t *)new_history, 0, size);
        wmemcpy((wchar_t *)new_history, le->history[le->history_len - to_copy], to_copy);
        free(le->history);
        le->history = new_history;
    }
    le->history_max_size = size;
    if (le->history_len > le->history_max_size)
        le->history_len = le->history_max_size;
}

/**
 * Store the history in a file.
 *
 * \return \c true if succeeded, and \c false otherwise.
 */
bool tim_line_edit_history_save(tim_line_edit_t *le, const char *path)
{
    assert(le);
    assert(path && *path && "History file path must not be empty.");

    const mode_t old_umask = umask(S_IXUSR | S_IRWXG | S_IRWXO);

    FILE *fp = fopen(path, "w");
    if (!fp)
    {
        TIM_TRACE(Error, "Failed to open History file '%s' for writing: %s",
                  path, strerror(errno));
        return false;
    }
    umask(old_umask);
    chmod(path, S_IRUSR | S_IWUSR);
    for (int i = 0; i < le->history_len; ++i)
    {
        char *line;
        tim_from_ws(&line, le->history[i]);
        fputs(line, fp);
        free(line);
    }
    fclose(fp);

    return true;
}

/**
 * Load the history from a file.
 *
 * \return \c true if succeeded, and \c false otherwise.
 */
bool tim_line_edit_history_load(tim_line_edit_t *le, const char *path)
{
    assert(le);
    assert(path && *path && "History file path must not be empty.");

    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        TIM_TRACE(Error, "Failed to open History file '%s' for reading: %s",
                  path, strerror(errno));
        return false;
    }

    char buf[le->buf_size * sizeof(wchar_t)];

    while (fgets(buf, sizeof(buf), fp))
    {
        char *p = strchr(buf,'\r');
        if (!p)
            p = strchr(buf,'\n');
        if (p)
            *p = '\0';
        wchar_t *wbuf;
        tim_to_ws(&wbuf, buf);
        tim_line_edit_history_add(le, wbuf);
        free(wbuf);
    }
    fclose(fp);

    return true;
}


// Static

/** Beep, used for completion when there is nothing to complete or when all
  * the choices were already shown. This method does nothing.
  * FIXME!
  */
static void tim_line_edit_beep(tim_line_edit_t *le)
{
    assert(le);
}

/* Called by complete_line() and show() to render the current
 * edited line with the proposed completion. If the current completion table
 * is already available, it is passed as second argument, otherwise the
 * function will use the callback to obtain it.
 *
 * Flags are the same as refresh_line*(). */
static void tim_line_edit_refresh_line_with_completion(tim_line_edit_t *le,
                                                       const tim_line_edit_completions_t *c,
                                                       unsigned refresh_flags)
{
    assert(le);
    assert(le->complete);

    /* Obtain the table of completions if the caller didn't provide one. */
    tim_line_edit_completions_t lc = { 0, NULL };
    if (!c)
    {
        char *buf;
        tim_from_ws(&buf, le->buf);
        le->complete(le, buf, &lc);
        free(buf);
        c = &lc;
    }

    /* Show the edited line with completion if possible, or just refresh. */
    if (le->completion_idx < c->size)
    {
        tim_line_edit_t saved = *le;

        le->len = le->pos = tim_to_ws(&le->buf, c->completions[le->completion_idx]);
        tim_line_edit_refresh_line_with_flags(le, refresh_flags);
        free(le->buf);

        le->len = saved.len;
        le->pos = saved.pos;
        le->buf = saved.buf;
    }
    else
        tim_line_edit_refresh_line_with_flags(le, refresh_flags);

    if (c != &lc)
        free(lc.completions);
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
static char tim_line_edit_complete_line(tim_line_edit_t *le, int key_pressed)
{
    assert(le);
    assert(le->complete);

    char c = key_pressed;

    tim_line_edit_completions_t lc;

    {
        char *line = NULL;
        tim_from_ws(&line, le->buf);
        le->complete(le, line, &lc);
        free(line);
    }
    if (!lc.size)
    {
        tim_line_edit_beep(le);
        le->in_completion = false;
        c = 0; // Never allow tabs.
    }
    else
    {
        switch (c)
        {
            case TimKeyTab:
                if (!le->in_completion)
                {
                    le->in_completion = true;
                    le->completion_idx = 0;
                }
                else
                {
                    le->completion_idx = (le->completion_idx + 1) % (lc.size + 1);
                    if (le->completion_idx == lc.size)
                        tim_line_edit_beep(le);
                }
                c = 0;
                break;

            case TimKeyEsc:
                /* Re-show original buffer. */
                if (le->completion_idx < lc.size)
                    tim_line_edit_refresh_line(le);
                le->in_completion = false;
                c = 0;
                break;

            default:
                /* Update buffer and return. */
                if (le->completion_idx < lc.size)
                    le->len
                        = le->pos
                        = mbstowcs(le->buf,
                                   lc.completions[le->completion_idx],
                                   le->buf_size - 1);
                le->in_completion = false;
                break;
        }

        /* Show completion or original buffer. */
        if (le->in_completion
                && le->completion_idx < lc.size)
            tim_line_edit_refresh_line_with_completion(le, &lc, TimRefreshAll);
        else
            tim_line_edit_refresh_line(le);
    }

    return c; /* Return last read character */
}

/** Helper of refresh_single_line() and refresh_multi_line() to show hints
  * to the right of the prompt.
  */
static void tim_line_edit_refresh_show_hints(tim_line_edit_t *le, tim_wstr_t *s)
{
    assert(le);
    assert(s);

    if (!le->hint)
        return;

    if (le->plen + le->len < le->cols)
    {
        int color = -1;
        bool bold = false;
        char *hint = NULL;
        {
            char *line;
            tim_from_ws(&line, le->buf);
            hint = le->hint(le, line, &color, &bold);
            free(line);
        }
        if (!hint
                && *hint)
        {
            wchar_t *whint = NULL;
            tim_to_ws(&whint, hint);
            free(hint);

            if (bold
                    && color == -1)
                color = 37;
            if (color != -1
                    || bold)
                tim_wcscat_swprintf(s, L"\033[%d;%d;49m", bold, color);

            tim_wcscat(s, whint);
            free(whint);

            if (color != -1
                    || bold)
                tim_wcscat(s, L"\033[0m");
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
static void tim_line_edit_refresh_single_line(tim_line_edit_t *le, unsigned refresh_flags)
{
    assert(le);

    const wchar_t *buf = le->buf;
    size_t len = le->len;
    size_t pos = le->pos;

    while (le->plen + pos >= le->cols)
    {
        ++buf;
        --len;
        --pos;
    }

    while (le->plen + len > le->cols)
        --len;

    /* Cursor to the left edge. */
    tim_wstr_t *ws = tim_wstr_new(L"\r");

    if ((refresh_flags & TimRefreshWrite))
    {
        /* Write the prompt and the current buffer content. */
        tim_wcsncat(ws, le->prompt, le->psize);
        if (le->mask_mode)
            tim_wcscat_fill(ws, '*', len);
        else
            tim_wcsncat(ws, buf, len);
        /* Show hits if any. */
        tim_line_edit_refresh_show_hints(le, ws);
    }

    /* Erase to right. */
    tim_wcscat(ws, L"\x1b[0K");

    if ((refresh_flags & TimRefreshWrite))
    {
        /* Move cursor to the original position. */
        tim_wcscat_swprintf(ws, L"\r\x1b[%dC", (int)(pos + le->plen));
    }

    {
        char *s = NULL;
        const size_t l = tim_from_ws(&s, tim_wcstr(ws));
        if (le->write(le->user_data, s, l) < 0)
        {
            /* Can't recover from write error. */
        }
        free(s);
    }
    tim_wstr_free(ws);
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 *
 * The function can just remove the old prompt, just write it, or both.
 */
static void tim_line_edit_refresh_multi_line(tim_line_edit_t *le, unsigned refresh_flags)
{
    assert(le);

    int rows = (int)((le->plen + le->len + le->cols - 1) / le->cols); /* Rows used by current buf. */
    int rpos = (int)((le->plen + le->old_pos + le->cols) / le->cols); /* Cursor relative row. */
    int rpos2; /* rpos after refresh. */
    int col; /* Colum position, zero-based. */
    int old_rows = (int)le->old_rows;
    int j;

    le->old_rows = rows;

    /* First step: clear all the lines used before. To do so start by
     * going to the last row. */

    tim_wstr_t *ws = tim_wstr_new(NULL);

    if ((refresh_flags & TimRefreshClean))
    {
        if (old_rows - rpos > 0)
            tim_wcscat_swprintf(ws, L"\x1b[%dB", old_rows - rpos);

        /* Now for every row clear it, go up. */
        for (j = 0; j < old_rows - 1; ++j)
            tim_wcscat(ws, L"\r\x1b[0K\x1b[1A");
    }

    if ((refresh_flags & TimRefreshAll))
    {
        /* Clean the top line. */
        tim_wcscat(ws, L"\r\x1b[0K");
    }

    if ((refresh_flags & TimRefreshWrite))
    {
        /* Write the prompt and the current buffer content. */
        tim_wcsncat(ws, le->prompt, le->psize);
        if (le->mask_mode)
            tim_wcscat_fill(ws, L'*', le->len);
        else
            tim_wcsncat(ws, le->buf, le->len);

        /* Show hits if any. */
        tim_line_edit_refresh_show_hints(le, ws);

        /* If we are at the very end of the screen with our prompt, we need to
         * emit a newline and move the prompt to the first column. */
        if (le->pos
                && le->pos == le->len
                && (le->pos + le->plen) % le->cols == 0)
        {
            tim_wcscat(ws, L"\n\r");
            ++rows;
            if (rows > (int)le->old_rows)
                le->old_rows = rows;
        }

        /* Move cursor to right position. */
        rpos2 = (int)((le->plen + le->pos + le->cols) / le->cols); /* Current cursor relative row. */

        /* Go up till we reach the expected position. */
        if (rows - rpos2 > 0)
            tim_wcscat_swprintf(ws, L"\x1b[%dA", rows - rpos2);

        /* Set column. */
        col = (le->plen + (int)le->pos) % (int)le->cols;
        if (col)
            tim_wcscat_swprintf(ws, L"\r\x1b[%dC", col);
        else
            tim_wcscat(ws, L"\r");
    }

    le->old_pos = le->pos;

    {
        char *s = NULL;
        const size_t l = tim_from_ws(&s, tim_wcstr(ws));
        if (le->write(le->user_data, s, l) < 0)
        {
            /* Can't recover from write error. */
        }
        free(s);
    }

    tim_wstr_free(ws);
}

/** Calls the two low level functions refresh_single_line() or
  * refresh_multi_line() according to the selected mode.
  */
static void tim_line_edit_refresh_line_with_flags(tim_line_edit_t *le, unsigned refresh_flags)
{
    assert(le);

    if (le->ml_mode)
        tim_line_edit_refresh_multi_line(le, refresh_flags);
    else
        tim_line_edit_refresh_single_line(le, refresh_flags);
}

/** Utility function to avoid specifying refresh_flagg::All all the times.
  */
static void tim_line_edit_refresh_line(tim_line_edit_t *le)
{
    assert(le);

    tim_line_edit_refresh_line_with_flags(le, TimRefreshAll);
}

/** Insert the character \a c at cursor current position.
 *
 * On error writing to the terminal \c false is returned, otherwise \c true.
 */
static bool tim_line_edit_insert(tim_line_edit_t *le, char c)
{
    assert(le);

    if (le->len < le->buf_size)
    {
        if (le->len == le->pos)
        {
            le->buf[le->pos] = c;
            ++(le->pos);
            ++(le->len);
            le->buf[le->len] = L'\0';
            if (!le->ml_mode
                    && le->plen + le->len < le->cols
                    && !le->hint)
            {
                /* Avoid a full update of the line in the
                 * trivial case. */
                char d = le->mask_mode
                            ? '*'
                            : c;
                if (le->write(le->user_data, &d, 1) != 1)
                    return false;
            }
            else
                tim_line_edit_refresh_line(le);
        }
        else
        {
            wmemmove(le->buf + le->pos + 1, le->buf + le->pos, le->len - le->pos);
            le->buf[le->pos] = c;
            ++(le->len);
            ++(le->pos);
            le->buf[le->len] = L'\0';
            tim_line_edit_refresh_line(le);
        }

        return true;
    }

    return false;
}

/** Move cursor on the left.
 */
static void tim_line_edit_move_left(tim_line_edit_t *le)
{
    assert(le);

    if (le->pos > 0)
    {
        --(le->pos);
        tim_line_edit_refresh_line(le);
    }
}

/** Move cursor on the right.
 */
static void tim_line_edit_move_right(tim_line_edit_t *le)
{
    assert(le);

    if (le->pos != le->len)
    {
        ++(le->pos);
        tim_line_edit_refresh_line(le);
    }
}

/** Move cursor to the start of the line.
 */
static void tim_line_edit_move_home(tim_line_edit_t *le)
{
    assert(le);

    if (le->pos)
    {
        le->pos = 0;
        tim_line_edit_refresh_line(le);
    }
}

/** Move cursor to the end of the line.
 */
static void tim_line_edit_move_end(tim_line_edit_t *le)
{
    assert(le);

    if (le->pos != le->len)
    {
        le->pos = le->len;
        tim_line_edit_refresh_line(le);
    }
}

/**
 * Add line to the history.
 */
static void tim_line_edit_history_add(tim_line_edit_t *le, const wchar_t *line)
{
    assert(le);
    assert(line);

    wchar_t *line_copy;

    if (le->history_max_size == 0)
        return;

    /* Initialization on first call. */
    if (!le->history)
    {
        le->history = calloc(1, sizeof(wchar_t *) * le->history_max_size);
        assert(le->history && "Failed to allocate memory for History.");
    }

    /* Don't add duplicated lines. */
    if (le->history_len
            && !wcscmp(le->history[le->history_len - 1], line))
        return;

    /* Add an heap allocated copy of the line in the history.
     * If we reached the max length, remove the older line. */
    line_copy = wcsdup(line);
    assert(line_copy);

    if (le->history_len == le->history_max_size)
    {
        free(le->history[0]);
        wmemmove(le->history[0], le->history[1], le->history_max_size - 1);
        --(le->history_len);
    }
    le->history[le->history_len] = line_copy;
    ++(le->history_len);
}

/** Substitute the currently edited line with the next or previous history
 * entry as specified by \a dir.
 */
static void tim_line_edit_history_next(tim_line_edit_t *le, tim_line_edit_history_dir_t dir)
{
    assert(le);

    if (le->history_len > 1)
    {
        /* Update the current history entry before to
         * overwrite it with the next one. */
        free(le->history[le->history_len - 1 - le->history_idx]);
        le->history[le->history_len - 1 - le->history_idx] = wcsdup(le->buf);
        /* Show the new entry */
        le->history_idx += (dir == TimLineEditHistoryPrev)
                                ? 1
                                : -1;
        if (le->history_idx < 0)
        {
            le->history_idx = 0;
            return;
        }

        if (le->history_idx >= le->history_len)
        {
            le->history_idx = le->history_len - 1;
            return;
        }

        wcpncpy(le->buf, le->history[le->history_len - 1 - le->history_idx], le->buf_size);
        le->buf[le->buf_size - 1] = '\0';
        le->len = le->pos = wcslen(le->buf);
        tim_line_edit_refresh_line(le);
    }
}

/** Delete the character at the right of the cursor without altering the cursor
  * position. Basically this is what happens with the [Delete] keyboard key.
  */
static void tim_line_edit_delete(tim_line_edit_t *le)
{
    assert(le);

    if (le->len > 0
            && le->pos < le->len)
    {
        wmemmove(le->buf + le->pos, le->buf + le->pos + 1, le->len - le->pos - 1);
        --(le->len);
        le->buf[le->len] = '\0';
        tim_line_edit_refresh_line(le);
    }
}

/** Backspace implementation.
 */
static void tim_line_edit_backspace(tim_line_edit_t *le)
{
    assert(le);

    if (le->pos > 0
            && le->len > 0)
    {
        wmemmove(le->buf + le->pos - 1, le->buf + le->pos, le->len - le->pos);
        --(le->pos);
        --(le->len);
        le->buf[le->len] = '\0';
        tim_line_edit_refresh_line(le);
    }
}

/** Delete the previous word, maintaining the cursor at the start of the
  * current word.
  */
static void tim_line_edit_delete_prev_word(tim_line_edit_t *le)
{
    assert(le);

    size_t old_pos = le->pos;
    size_t diff;

    while (le->pos > 0
                && le->buf[le->pos - 1] == L' ')
        --(le->pos);
    while (le->pos > 0
                && le->buf[le->pos - 1] != L' ')
        --(le->pos);
    diff = old_pos - le->pos;
    wmemmove(le->buf + le->pos, le->buf + old_pos, le->len - old_pos + 1);
    le->len -= diff;
    tim_line_edit_refresh_line(le);
}
