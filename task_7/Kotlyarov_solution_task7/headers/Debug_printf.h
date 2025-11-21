#ifndef DEBUG_PRINTF_H
#define DEBUG_PRINTF_H

#ifndef NO_DEBUG_PRINTF
#include <stdio.h>
#define DEBUG_PRINTF(...)  fprintf(stderr, __VA_ARGS__);
#else
#define DEBUG_PRINTF(...);
#endif

#endif
