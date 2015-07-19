#pragma once

namespace lct {

enum LogLevel {
	DEBUG, INFO, WARNING, ERROR
};

#define LOG_DEBUG(...) lct::log(lct::DEBUG, __func__, __VA_ARGS__)
#define LOG_INFO(...) lct::log(lct::INFO, __func__, __VA_ARGS__)
#define LOG_WARNING(...) lct::log(lct::WARNING, __func__, __VA_ARGS__)
#define LOG_ERROR(...) lct::log(lct::ERROR, __func__, __VA_ARGS__)

void log(LogLevel level, const char* func, const char* format, ...)
	__attribute__((format(printf,3,4)));

} /* namespace lct */
