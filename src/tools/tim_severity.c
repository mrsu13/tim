#include "tim_severity.h"

#include <stddef.h>


const char *tim_severity_title(tim_severity_t severity)
{
    switch (severity)
    {
        case TimError:
            return "Error";
        case TimWarning:
            return "Warning";
        case TimInfo:
            return "Info";
        case TimDebug:
            return "Debug";
        case TimTrace:
            return "Trace";
    }

    return NULL;
}
