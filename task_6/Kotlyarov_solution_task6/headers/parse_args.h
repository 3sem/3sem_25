#ifndef PARSE_ARGS
#define PARSE_ARGS

#include <stdbool.h>
#include <string.h>
#include "Daemon_config.h"

#define DEFAULT_SAMPLE_INTERVAL 1000000U  /* 1 s */
#define MIN_TIME_INTERVAL 1000U           /* 1 ms */
#define MAX_TIME_INTERVAL 60000000U       /* 60 s */

bool parse_args(int argc, char* argv[], struct Daemon_cfg* cfg);
void print_usage(const char* const program_name);

#endif
