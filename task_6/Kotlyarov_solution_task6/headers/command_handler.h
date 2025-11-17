#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <ctype.h>
#include <string.h>
#include "monitor_modes.h"

typedef enum {
    OUTPUT_CONSOLE,
    OUTPUT_LOG
} OutputMode;

void handle_command(const char* cmd, struct Daemon_cfg* config, OutputMode output_mode);

char* sanitize_command(const char* input);

void handle_status_command(struct Daemon_cfg* config, OutputMode output_mode);
void handle_interval_command(const char* args, struct Daemon_cfg* config, OutputMode output_mode);
void handle_pid_command(const char* args, struct Daemon_cfg* config, OutputMode output_mode);
void handle_backup_command(struct Daemon_cfg* config, OutputMode output_mode);
void handle_stop_command(struct Daemon_cfg* config, OutputMode output_mode);
void handle_help_command(OutputMode output_mode);
void handle_reload_command(struct Daemon_cfg* config, OutputMode output_mode);

void print_to_output(const char* message, OutputMode output_mode);
void update_target_pid(struct Daemon_cfg* config, pid_t new_pid);

#endif
