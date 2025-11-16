#ifndef MONITOR_MODES
#define MONITOR_MODES

#define INIT_MAPS_COUNT 32
#define MAX_MAPS_PATH_LEN 64
#define MAX_LINE_LENGTH 256

#include "parse_maps.h"

int ready_to_read(void);
void set_nonblocking_mode(int enable);
void handle_interactive_command(const char* cmd, struct Daemon_cfg* config);
int interactive_mode(struct Daemon_cfg* config);
void print_changes(struct Maps_snapshot* old, struct Maps_snapshot* new);

#endif
