#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


bool tim_file_exists(const char *path);
bool tim_file_read(const char *path, uint8_t **buffer, size_t *size, size_t max_size);
