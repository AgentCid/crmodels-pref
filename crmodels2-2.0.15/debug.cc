#include <stdio.h>
#include <stdlib.h>

#include "compile-flags.h"

#include "debug.h"

#include "timed_computation.h"

void printTimestamp(const char *fmt, ...)
{
#ifdef PRINT_TIMESTAMPS
	va_list ap;

	va_start(ap,fmt);

	fprintf(stderr,"-------");
	vfprintf(stderr,fmt,ap);
	fprintf(stderr,": %s\n",get_total_cputime_string());

	va_end(ap);
#endif
}
