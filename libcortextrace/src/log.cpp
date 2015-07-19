#include "log.h"

#include <cstdio>
#include <cstdarg>
#include <iostream>

namespace lct {

void log(LogLevel level, const char* func, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0';

    va_end(args);

    std::cerr << "--- " << func << ": " << buffer << std::endl;
}

} /* namespace lct */
