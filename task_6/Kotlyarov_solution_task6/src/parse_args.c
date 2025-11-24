#include "parse_args.h"

bool parse_args(int argc, char* argv[], struct Daemon_cfg* cfg) {
	if (!argv || !cfg) {
		DEBUG_PRINTF("ERROR: null pointer passed to parse_args\n");
		return false;
	}

	if (argc < 2) {
        print_usage(argv[0]);
        return false;
    }

    cfg->target_pid = 0;
    cfg->mode = NO_MODE;
    cfg->is_running = 1;
    cfg->last_snapshot = NULL;

	for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            char *endptr;
            long pid_value = strtol(argv[++i], &endptr, 10);
            if (*endptr != '\0' || pid_value <= 0) {
                printf("Invalid PID: %s\n", argv[i]);
                return false;
            }
            cfg->target_pid = (pid_t)pid_value;
        }
        else if (strcmp(argv[i], "-i") == 0) {
            cfg->mode = INTERACTIVE_MODE;
        }
        else if (strcmp(argv[i], "-d") == 0) {
            cfg->mode = DAEMON_MODE;
        }
        else if (strcmp(argv[i], "-T") == 0 && i + 1 < argc) {
            char *endptr;
            long time_value = strtol(argv[++i], &endptr, 10);
            if (*endptr != '\0' || time_value <= 0) {
                printf("Invalid time: %s\n", argv[i]);
                return false;
            }
            cfg->sample_interval = (useconds_t)time_value;
        }
        else {
            printf("Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return false;
        }
    }

    if (cfg->target_pid == 0) {
        printf("Error: PID is required\n");
        print_usage(argv[0]);
        return false;
    }

    if (cfg->mode == NO_MODE) {
        printf("Error: mode (-i or -d) is required\n");
        print_usage(argv[0]);
        return false;
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
