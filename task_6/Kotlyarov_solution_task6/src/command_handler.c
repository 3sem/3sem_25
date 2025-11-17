#include "command_handler.h"

void print_to_output(const char* message, OutputMode output_mode) {
    if (output_mode == OUTPUT_CONSOLE) {
        printf("%s\n", message);
    } else {
        log_message("CMD: %s", message);
    }
}

char* sanitize_command(const char* input) {
    if (!input) return NULL;
    
    static char clean[256];
    strncpy(clean, input, sizeof(clean) - 1);
    clean[sizeof(clean) - 1] = '\0';
    
    char* start = clean;
    while (*start && isspace(*start)) start++;
    
    char* end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) end--;
    *(end + 1) = '\0';
    
    return strlen(start) > 0 ? start : NULL;
}

void handle_status_command(struct Daemon_cfg* config, OutputMode output_mode) {
    if (output_mode == OUTPUT_CONSOLE) {
        printf("Current PID: %d\n", config->target_pid);
        printf("Sample interval: %u us\n", config->sample_interval);
        printf("Status: %s\n", config->is_running ? "running" : "stopped");
    } else {
        log_message("STATUS: PID=%d, Interval=%uus, Running=%d", 
                   config->target_pid, config->sample_interval, config->is_running);
    }
}

void handle_interval_command(const char* args, struct Daemon_cfg* config, OutputMode output_mode) {
    unsigned long new_interval = atol(args);
    if (new_interval > 0 && new_interval <= 3600000000UL) {
        config->sample_interval = new_interval;
        print_to_output("Interval changed successfully", output_mode);
        log_message("Interval changed to: %lu us", new_interval);
    } else {
        print_to_output("Error: Invalid interval value", output_mode);
        log_message("Error: Invalid interval value: %s", args);
    }
}

void update_target_pid(struct Daemon_cfg* config, pid_t new_pid) {
    config->target_pid = new_pid;
    
    if (config->last_snapshot) {
        free_maps_snapshot(config->last_snapshot);
    }
    
    config->last_snapshot = read_maps_snapshot(new_pid);
    if (config->last_snapshot) {
        save_full_backup(config->last_snapshot, new_pid);
    }
}

void handle_pid_command(const char* args, struct Daemon_cfg* config, OutputMode output_mode) {
    pid_t new_pid = atoi(args);
    if (new_pid > 0) {
        if (kill(new_pid, 0) == 0) {
            update_target_pid(config, new_pid);
            print_to_output("PID changed successfully", output_mode);
            log_message("Target PID changed to %d", new_pid);
        } else {
            print_to_output("Error: Process does not exist or inaccessible", output_mode);
            log_message("Error: Process %d does not exist or inaccessible", new_pid);
        }
    } else {
        print_to_output("Error: Invalid PID", output_mode);
        log_message("Error: Invalid PID: %s", args);
    }
}

void handle_backup_command(struct Daemon_cfg* config, OutputMode output_mode) {
    if (config->last_snapshot) {
        save_full_backup(config->last_snapshot, config->target_pid);
        print_to_output("Manual backup created", output_mode);
        log_message("Manual backup created via command");
    } else {
        print_to_output("Error: No snapshot available for backup", output_mode);
        log_message("Error: No snapshot available for backup");
    }
}

void handle_stop_command(struct Daemon_cfg* config, OutputMode output_mode) {
    print_to_output("Stopping...", output_mode);
    log_message("Stop command received");
    config->is_running = 0;
}

void handle_reload_command(struct Daemon_cfg* config, OutputMode output_mode) {
    reload_configuration(config);
    
    if (output_mode == OUTPUT_CONSOLE) {
        printf("Configuration reloaded successfully\n");
    } else {
        log_message("Configuration reloaded via command");
    }
}

void handle_help_command(OutputMode output_mode) {
    if (output_mode == OUTPUT_CONSOLE) {
        printf("\nAvailable commands:\n");
        printf("  status          - current status\n");
        printf("  interval <us>   - change sample time\n");
        printf("  pid <new_pid>   - change target PID\n");
        printf("  backup          - create manual backup\n");
        printf("  reload          - reload configuration\n");
        printf("  help            - this reference\n");
        printf("  quit            - exit\n");
    } else {
        log_message("HELP: Available commands: status, interval, pid, backup, reload, help, quit");
    }
}

void handle_command(const char* cmd, struct Daemon_cfg* config, OutputMode output_mode) {
    char* clean_cmd = sanitize_command(cmd);
    if (!clean_cmd || !config) return;

    if (output_mode == OUTPUT_CONSOLE) {
        printf("\n>>> Command: %s\n", clean_cmd);
    }

    if (strcmp(clean_cmd, "q") == 0 || strcmp(clean_cmd, "quit") == 0 || strcmp(clean_cmd, "stop") == 0) {
        handle_stop_command(config, output_mode);
    }
    else if (strcmp(clean_cmd, "status") == 0) {
        handle_status_command(config, output_mode);
    }
    else if (strncmp(clean_cmd, "interval ", 9) == 0) {
        handle_interval_command(clean_cmd + 9, config, output_mode);
    }
    else if (strncmp(clean_cmd, "pid ", 4) == 0) {
        handle_pid_command(clean_cmd + 4, config, output_mode);
    }
    else if (strcmp(clean_cmd, "backup") == 0 || strcmp(clean_cmd, "snapshot") == 0) {
        handle_backup_command(config, output_mode);
    }
    else if (strcmp(clean_cmd, "reload") == 0) {
        handle_reload_command(config, output_mode);
    }
    else if (strcmp(clean_cmd, "help") == 0) {
        handle_help_command(output_mode);
    }
    else {
        print_to_output("Unknown command. Type 'help' for reference.", output_mode);
        log_message("Unknown command: '%s'", clean_cmd);
    }

    if (output_mode == OUTPUT_CONSOLE) {
        printf(">>> ");
        fflush(stdout);
    }
}
