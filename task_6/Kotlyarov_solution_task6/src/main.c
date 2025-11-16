#include "monitor_modes.h"
#include "parse_args.h"

int main(int argc, char* argv[]) {
    struct Daemon_cfg cfg = {
        .target_pid = 0,
        .sample_interval = DEFAULT_SAMPLE_INTERVAL,
        .mode = NO_MODE,
        .is_running = 1,
        .last_snapshot = NULL
    };
    
    if(!parse_args(argc, argv, &cfg)) {
        return 1;
    }

    int result = 0;
    if (cfg.mode == INTERACTIVE_MODE) {
        result = interactive_mode(&cfg);
    } else {
        printf("cfg.mode = %d\n", cfg.mode);
    }
    
    if (cfg.last_snapshot) {
        free_maps_snapshot(cfg.last_snapshot);
        cfg.last_snapshot = NULL;
    }
    
    return result;
}
