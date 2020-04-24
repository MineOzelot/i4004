#include "error.h"

int position_vprintf(position pos, enum e_log_level lvl, const char *__restrict fmt, va_list va) {
	const char *level = 0;
	FILE *stream = 0;
	switch(lvl) {
		case LOG_ERROR:
			level = "error";
			stream = stderr;
			break;
		case LOG_WARNING:
			level = "warning";
			stream = stderr;
			break;
	}
	int ret = fprintf(stderr, "%s:%zu:%zu: %s: ", pos.filename, pos.line, pos.column, level);
	if(ret < 0) return ret;

	ret += vfprintf(stream, fmt, va);
	return ret;
}
