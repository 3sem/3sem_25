#ifndef MONITOR_MODES
#define MONITOR_MODES


#include <sys/select.h>
#include <termios.h>
#include "parse_maps.h"

int ready_to_read(void);
void set_nonblocking_mode(int enable);
void handle_interactive_command(const char* cmd, struct Daemon_cfg* config);
int interactive_mode(struct Daemon_cfg* config);
void print_changes(struct Maps_snapshot* old, struct Maps_snapshot* new);

#endif
