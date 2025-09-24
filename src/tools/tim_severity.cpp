#include "tim_severity.h"


const char *tim::severity_title(tim::severity severity)
{
    switch (severity)
    {
        case tim::severity::Fatal:
            return "[F]";
        case tim::severity::Error:
            return "[E]";
        case tim::severity::Warning:
            return "[W]";
        case tim::severity::Info:
            return "[I]";
        case tim::severity::Debug:
            return "[D]";
        case tim::severity::Trace:
            return "[T]";
    }

    return nullptr;
}
