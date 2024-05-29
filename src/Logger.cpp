#ifndef LOGGER_H
#include "../inc/Logger.h"
#include <cstdarg>
#include <stdio.h>

Logger::Logger() {}

Logger::~Logger() {}

Logger *Logger::get_instance() {
    if (instance == nullptr)
        instance = new Logger();

    return instance;
}

void Logger::log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

#endif