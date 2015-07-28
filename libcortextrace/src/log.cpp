#include "log.h"

#include <cstdio>
#include <cstdarg>
#include <iostream>
#include <sys/time.h>

namespace lct {

static const char* levelStr(LogLevel level)
{
    switch (level) {
    case DEBUG: return "debug";
    case INFO: return "info";
    case WARNING: return "warning";
    case ERROR: return "error";
    }
}

void log(LogLevel level, const char* func, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0';

    va_end(args);

    struct timeval now;
    gettimeofday(&now, NULL);

    std::cerr << now.tv_sec << "." << now.tv_usec / 1000
            << " [" << levelStr(level) << "] " << func << ": " << buffer << std::endl;
}

} /* namespace lct */
