#pragma once

typedef enum tim_severity
{
    TimError,
    TimWarning,
    TimInfo,
    TimDebug,
    TimTrace
} tim_severity_t;

const char *tim_severity_title(tim_severity_t severity);
