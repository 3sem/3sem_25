#ifndef MONITOR_MODES
#define MONITOR_MODES


#include <sys/select.h>
#include <termios.h>
#include "parse_maps.h"
#include "backup_funcs.h"

int ready_to_read(void);
void set_nonblocking_mode(int enable);
int process_snapshot(struct Daemon_cfg* config);
int check_target_process(pid_t pid);
int daemon_init(void);
void check_fifo_commands(int fifo_fd, struct Daemon_cfg* config);
int interactive_mode(struct Daemon_cfg* config);
void reload_configuration(struct Daemon_cfg* config);
int daemon_mode(struct Daemon_cfg* config);
void signal_handler(int sig);
void setup_signal_handlers(void);
void log_changes(const struct Maps_diff* diff, int interactive);
#endif
