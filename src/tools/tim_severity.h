#pragma once

namespace tim
{

enum class severity
{
    Fatal,
    Error,
    Warning,
    Info,
    Debug,
    Trace
};

const char *severity_title(tim::severity severity);

}
