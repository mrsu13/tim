#include "tim_file.h"

#include "tim_trace.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


// Public

bool tim_file_exists(const char *path)
{
    assert(path && *path && "File path must not be empty.");

    FILE *fp = fopen(path, "rb");
    const bool res = fp;
    fclose(fp);
    return res;
}

bool tim_file_read(const char *path, uint8_t **buffer, size_t *size, size_t max_size)
{
    assert(path && *path && "File path must not be empty.");
    assert(buffer);
    assert(size);

    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        TIM_TRACE(Error, "Failed to open file '%s'.", path);
        return false;
    }

    if (fseek(fp, 0L, SEEK_END))
    {
        TIM_TRACE(Error, "Failed to seek file '%s'.", path);
        fclose(fp);
        return false;
    }

    *size = ftell(fp);

    if (fseek(fp, 0L, SEEK_SET))
    {
        TIM_TRACE(Error, "Failed to seek file '%s'.", path);
        fclose(fp);
        return false;
    }

    if (max_size
            && *size > max_size)
    {
        TIM_TRACE(Error, "File '%s' is too big. The maximum size is %u bytes.",
                  path, (unsigned)max_size);
        fclose(fp);
        return false;
    }

    *buffer = (uint8_t *)calloc(1, *size);
    assert(*buffer && "Failed to allocate memory to load a file.");

    const size_t n = fread(*buffer, 1, *size, fp);
    fclose(fp);

    if (n != *size)
    {
        TIM_TRACE(Error, "Failed to read file '%s'.", path);
        free(*buffer);
        return false;
    }

    return true;
}
