#ifndef NO_DEBUG_PRINTF
#define DEBUG_PRINTF(...)  fprintf(stderr, __VA_ARGS__);
#else
#define DEBUG_PRINTF(...);
#endif
