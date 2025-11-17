#include "monitor_modes.h"
#include "parse_args.h"
#include "backup_funcs.h"

int main(int argc, char* argv[]) {
    struct Daemon_cfg cfg = {
        .target_pid = 0,
        .sample_interval = DEFAULT_SAMPLE_INTERVAL,
        .mode = NO_MODE,
        .is_running = 1,
        .last_snapshot = NULL
    };
    
    if (create_backup_dirs() != BACKUP_SUCCESS) {
        printf("Failed to create backup directories\n");
        return 1;
    }
    
    log_message("Memory monitor starting");
    
    if(!parse_args(argc, argv, &cfg)) {
        log_message("Failed to parse arguments");
        return 1;
    }
    
    log_message("Arguments parsed successfully. Mode: %d, PID: %d", cfg.mode, cfg.target_pid);
    
    int result = 0;
    if (cfg.mode == INTERACTIVE_MODE) {
        log_message("Entering interactive mode");
        result = interactive_mode(&cfg);
    } else if (cfg.mode == DAEMON_MODE) {
        log_message("Entering daemon mode");
        result = daemon_mode(&cfg);
    } else {
        printf("Unknown mode: %d\n", cfg.mode);
        log_message("Unknown mode: %d", cfg.mode);
    }
    
    if (cfg.last_snapshot) {
        free_maps_snapshot(cfg.last_snapshot);
        cfg.last_snapshot = NULL;
    }
    
    log_message("Memory monitor exiting");
    return result;
}
