#include "tim_severity.h"


/**
 * Возвращает короткую отображаемую метку уровня для печати в stderr:
 * "[F]" для fatal, "[E]" для error и т.д.
 */
const char *tim::severity_title(tim::severity severity)
{
    switch (severity)
    {
        case tim::severity::fatal:
            return "[F]";
        case tim::severity::error:
            return "[E]";
        case tim::severity::warning:
            return "[W]";
        case tim::severity::info:
            return "[I]";
        case tim::severity::debug:
            return "[D]";
        case tim::severity::trace:
            return "[T]";
    }

    return nullptr;
}
