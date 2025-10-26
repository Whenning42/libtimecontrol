#pragma once


__attribute__((__format__ (__printf__, 1, 2)))
void log(const char* fmt, ...);

__attribute__((__format__ (__printf__, 1, 2)))
void info(const char* fmt, ...);
