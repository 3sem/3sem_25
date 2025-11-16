#include "monitor_modes.h"

int main(int argc, char *argv[]) {

    struct daemon_cfg cfg = {};

    if(parse_args(argc, argv, &cfg))
        return 1;
    
    if (cfg->mode == INTERACTIVE_MODE) {
        interactive_mode(&cfg);
    } else {
    
    }

    return 0;
}
