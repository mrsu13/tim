#include "tim_severity.h"


const char *tim::severity_title(tim::severity severity)
{
    switch (severity)
    {
        case tim::severity::Fatal:
            return "Fatal";
        case tim::severity::Error:
            return "Error";
        case tim::severity::Warning:
            return "Warning";
        case tim::severity::Info:
            return "Info";
        case tim::severity::Debug:
            return "Debug";
        case tim::severity::Trace:
            return "Trace";
    }

    return nullptr;
}
