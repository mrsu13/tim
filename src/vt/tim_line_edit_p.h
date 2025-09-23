#pragma once

#include "tim_line_edit.h"

#include "utringbuffer.h"


typedef struct tim_line_edit_completions
{
    size_t size;
    char **completions;
} tim_line_edit_completions_t;

typedef void (tim_line_edit_completer_t)(void * data, const char *prefix, tim_line_edit_completions_t *completions);

/**
 * Если этот колбэк задан, то его результат будет показан справа
 * от промпта перед редактируемой пользователем строкой.
 */
typedef char *(tim_line_edit_hinter_t)(void *data, const char *line, int *color, bool *bold);

typedef struct tim_line_edit
{
    tim_line_edit_write_t write;
    void *user_data;

    tim_line_edit_completer_t *complete;
    tim_line_edit_hinter_t *hint;

    bool in_completion; /* The user pressed `Tab` and we are now in completion
                         * mode, so input is handled by complete_line(). */
    size_t completion_idx; /* Index of next completion to propose. */
    char *buf; /* Edited line buffer. */
    size_t buf_size; /* Edited line buffer size. */
    size_t len; /* Current edited line length in characters. */
    char *prompt; /* Prompt to display. */
    size_t plen; /* Prompt length. We calculate decolorized prompt length in characters here. */
    size_t psize; /* Prompt size in bytes. */
    size_t pos; /* Current cursor position. */
    size_t old_pos; /* Previous refresh cursor position. */
    size_t cols; /* Number of columns in terminal. */
    size_t old_rows; /* Rows used by last refreshed line (multiline mode) */

    UT_ringbuffer *history;
    int history_idx; /* The history index we are currently editing. */

    bool mask_mode; /* Show "***" instead of input. For passwords. */
    bool ml_mode; /* Multi line mode. Default is single line. */
} tim_line_edit_t;
