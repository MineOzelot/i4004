#ifndef I4004_POSITION_H
#define I4004_POSITION_H

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

typedef struct {
	size_t line;
	size_t column;

	const char *filename;
} position;

enum e_log_level {
	LOG_ERROR
};

int position_vprintf(position pos, enum e_log_level lvl, const char *__restrict fmt, va_list va);

__attribute__(( format(printf, 3, 4) ))
static inline int position_printf(position pos, enum e_log_level lvl, const char *__restrict fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = position_vprintf(pos, lvl, fmt, va);
	va_end(va);
	return ret;
}

__attribute__(( format(printf, 2, 3) ))
static inline int position_error(position pos, const char *__restrict fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int ret = position_vprintf(pos, LOG_ERROR, fmt, va);
	va_end(va);
	return ret;
}

#endif //I4004_POSITION_H
