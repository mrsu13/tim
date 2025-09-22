#pragma once

#include <stdbool.h>
#include <stddef.h>


typedef struct tim_line_edit tim_line_edit_t;
typedef int (*tim_line_edit_write_t)(void *user_data, const char *data, size_t size);

void tim_line_edit_init(tim_line_edit_t *le, tim_line_edit_write_t write, void *user_data);
void tim_line_edit_destroy(tim_line_edit_t *le);

tim_line_edit_t *tim_line_edit_new(tim_line_edit_write_t write, void *user_data);
void tim_line_edit_free(tim_line_edit_t *le);

bool tim_line_edit_new_line(tim_line_edit_t *le, const char *prompt);

typedef enum tim_line_edit_status
{
    TimLineEditFinished, ///< Editing is finished. Now you can use the edited line.
    TimLineEditContinue, ///< Editing is in progress. You should call tim_line_edit_get_line() again.
    TimLineEditExit, ///< User pressed `[Ctrl+D]` or `[Ctrl+C]`.
    TimLineEditError ///< Reading from input stream or writing to output stream failed.
} tim_line_edit_status_t;

tim_line_edit_status_t tim_line_edit_get_line(tim_line_edit_t *le, const char *data, size_t size);

bool tim_line_edit_empty(const tim_line_edit_t *le);
char *tim_line_edit_line(const tim_line_edit_t *le);

void tim_line_edit_hide(tim_line_edit_t *le);
void tim_line_edit_show(tim_line_edit_t *le);

void tim_line_edit_set_term_width(tim_line_edit_t *le, size_t cols);
void tim_line_edit_set_multiline(tim_line_edit_t *le, bool enable);
void tim_line_edit_set_mask_mode(tim_line_edit_t *le, bool enable);

void tim_line_edit_clear_screen(tim_line_edit_t *le);

void tim_line_edit_history_set_max_size(tim_line_edit_t *le, size_t size);
bool tim_line_edit_history_save(tim_line_edit_t *le, const char *path);
bool tim_line_edit_history_load(tim_line_edit_t *le, const char *path);
