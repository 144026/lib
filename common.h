#pragma once
#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof (arr) / sizeof (arr)[0])
#endif

#ifndef offsetof
#define offsetof(a, b) __builtin_offsetof(a, b)
#endif

#ifndef container_of
#define container_of(p, type, field) ((type *) ((char *) (p) - offsetof(type, field)))
#endif

#ifndef min
#define min(a, b) ({typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#endif

#ifndef max
#define max(a, b) ({typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#endif


enum loglevel {
	LOGLEVEL_NONE,
	LOGLEVEL_ERROR,
	LOGLEVEL_WARN,
	LOGLEVEL_INFO,
	LOGLEVEL_DEBUG,
};
extern int logging;

#define debug(fmt, args...) do { if (logging >= LOGLEVEL_DEBUG) fprintf(stderr, "[DEBUG][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)
#define info(fmt, args...)  do { if (logging >= LOGLEVEL_INFO)  fprintf(stderr, "[INFO ][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)
#define warn(fmt, args...)  do { if (logging >= LOGLEVEL_WARN)  fprintf(stderr, "[WARN ][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)
#define error(fmt, args...) do { if (logging >= LOGLEVEL_ERROR) fprintf(stderr, "[ERROR][%s:%d] %-24s : " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); } while (0)


#ifndef __printf
#define __printf(x, y) __attribute__(( format(printf, x, y) ))
#endif

static inline __printf(3, 0) int vsnprintf_s(char *buf, size_t size, const char *format, va_list ap)
{
	int n = vsnprintf(buf, size, format, ap);

	if (n < 0)
		return 0;

	/* n >= size, no more space left */
	return n < size ? n :size;

}

static inline __printf(3, 4) int snprintf_s(char *buf, size_t size, const char *format, ...)
{
	int n;
	va_list ap;

	va_start(ap, format);
	n = vsnprintf_s(buf, size, format, ap);
	va_end(ap);

	return n;
}


#include "list.h"
#include "dynamic_array.h"
#include "dict.h"
