#ifndef __PRINTF_H
#define __PRINTF_H

#include <stdio.h>

#define printf(...) { \
	fprintf(stdout, __VA_ARGS__); \
	fflush(stdout); \
}

#endif
