#include "parse_args.h"

bool parse_args(int argc, char* argv[], struct Daemon_cfg* cfg) {
	if (!argv || !cfg) {
		DEBUG_PRINTF("ERROR: null pointer passed to parse_args\n");
		return false;
	}

	if (argc < 4 || strcmp(argv[1], "-p")) {
		print_usage(argv[0]);
		return false;
	}

	const char *mode_flag = argv[3];
	if (!strcmp(mode_flag, "-d")) {
		cfg->mode = DAEMON_MODE;
	} else if (!strcmp(mode_flag, "-i")) {
		cfg->mode = INTERACTIVE_MODE;
	} else {
		cfg->mode = NO_MODE;
		print_usage(argv[0]);
		return false;
	}

	char *endptr;
	long pid_value = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0' || pid_value <= 0) {
		DEBUG_PRINTF("Invalid PID: %s\n", argv[2]);
		print_usage(argv[0]);
		return false;
	}
	cfg->target_pid = (pid_t)pid_value;

	if (argc >= 6 && !strcmp(argv[4], "-T")) {
		long time_value = strtol(argv[5], &endptr, 10);
		if (*endptr != '\0' || time_value <= 0) {
			DEBUG_PRINTF("Invalid time: %s\n", argv[5]);
			print_usage(argv[0]);
            return false;
		} else if (time_value < MIN_TIME_INTERVAL || time_value > MAX_TIME_INTERVAL) {
            DEBUG_PRINTF("Wrong sample interval: time must belong tp [%u us;%u us]",
                          MIN_TIME_INTERVAL, MAX_TIME_INTERVAL); 
        }
		cfg->sample_interval = (useconds_t)time_value;
	} else {
		cfg->sample_interval = DEFAULT_SAMPLE_INTERVAL;
    }

	return true;
}

void print_usage(const char* const program_name) {
    DEBUG_PRINTF("Usage: %s -p <pid> -d/-i [-T <time_us>]\n", program_name);
    DEBUG_PRINTF("  -p <pid>    Target process ID (required)\n");
    DEBUG_PRINTF("  -d          Daemon mode\n");
    DEBUG_PRINTF("  -i          Interactive mode\n");
    DEBUG_PRINTF("  -T <time>   Sample interval in microseconds (optional)\n");
    DEBUG_PRINTF("              Default: %u us (1 second)\n", DEFAULT_SAMPLE_INTERVAL);
}
