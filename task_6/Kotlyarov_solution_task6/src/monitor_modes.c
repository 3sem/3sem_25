#include "monitor_modes.h"
#include "command_handler.h"

static struct Daemon_cfg* global_daemon_cfg = NULL;

void signal_handler(int sig) {
    if (!global_daemon_cfg) return;

    switch(sig) {
        case SIGTERM:
        case SIGINT:
            log_message("Received signal %d, shutting down", sig);
            global_daemon_cfg->is_running = 0;
            break;
        case SIGHUP:
            log_message("Received SIGHUP, reloading configuration");
            reload_configuration(global_daemon_cfg);
            break;
        case SIGUSR1:
            log_message("Received SIGUSR1, creating manual backup");
            if (global_daemon_cfg->last_snapshot) {
                save_full_backup(global_daemon_cfg->last_snapshot, global_daemon_cfg->target_pid);
            }
            break;
        default:
            log_message("Received unknown signal: %d", sig);
            break;
    }
}

void reload_configuration(struct Daemon_cfg* config) {
    if (!config) {
        log_message("ERROR: Cannot reload configuration - config is NULL");
        return;
    }
    
    log_message("Reloading configuration...");
    
    if (config->last_snapshot) {
        free_maps_snapshot(config->last_snapshot);
    }
    
    config->last_snapshot = read_maps_snapshot(config->target_pid);
    if (config->last_snapshot) {
        log_message("Configuration reloaded successfully - new snapshot taken");
        save_full_backup(config->last_snapshot, config->target_pid);
        log_message("New backup created after reload");
    } else {
        log_message("ERROR: Failed to reload configuration - cannot read snapshot");
    }
}

void setup_signal_handlers(void) {
    struct sigaction sa = {
        .sa_handler = signal_handler,
        .sa_flags = 0
    };
    sigemptyset(&sa.sa_mask);

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
}

int ready_to_read(void) {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

void set_nonblocking_mode(int enable) {
    static struct termios oldt, newt;
    
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

void log_changes(const struct Maps_diff* diff, int interactive) {
    if (!diff) return;

    size_t added_count = diff->added.size / sizeof(struct Memory_map);
    size_t removed_count = diff->removed.size / sizeof(struct Memory_map);
    size_t modified_count = diff->modified.size / sizeof(struct Memory_map);

    if (added_count > 0 || removed_count > 0 || modified_count > 0) {
        if (interactive) {
            printf("\n[Changes] +%zu added, -%zu removed, ~%zu modified\n", 
                   added_count, removed_count, modified_count);
        }
        log_message("Changes detected: +%zu added, -%zu removed, ~%zu modified",
                   added_count, removed_count, modified_count);
    }
}

int process_snapshot(struct Daemon_cfg* config) {
    struct Maps_snapshot* current = read_maps_snapshot(config->target_pid);
    if (!current) return -1;

    struct Maps_diff diff = {0};
    int diff_result = compare_snapshots(config->last_snapshot, current, &diff);
    
    if (diff_result == DIFF_SUCCESS) {
        size_t added_count = diff.added.size / sizeof(struct Memory_map);
        size_t removed_count = diff.removed.size / sizeof(struct Memory_map);
        size_t modified_count = diff.modified.size / sizeof(struct Memory_map);
        
        if (added_count > 0 || removed_count > 0 || modified_count > 0) {
            store_diff_history(config, &diff);
            save_incremental_backup(&diff, config->target_pid);
        } else {
            log_message("No changes detected, skipping history and backup");
        }
    }
    
    free_maps_diff(&diff);
    
    if (config->last_snapshot) {
        free_maps_snapshot(config->last_snapshot);
    }
    config->last_snapshot = current;
    
    return (diff_result == DIFF_SUCCESS) ? 0 : -1;
}

int check_target_process(pid_t pid) {
    return kill(pid, 0) == 0;
}

int daemon_init(void) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) return pid;

    setsid();
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    FILE* log_stream = fopen("logs/daemon_output.log", "a");
    if (log_stream) {
        dup2(fileno(log_stream), STDOUT_FILENO);
        dup2(fileno(log_stream), STDERR_FILENO);
    }
    
    return 0; 
}

void check_fifo_commands(int fifo_fd, struct Daemon_cfg* config) {
    char command[256] = {0};
    char buffer[256];
    ssize_t bytes_read;
    
    while ((bytes_read = read(fifo_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strncat(command, buffer, sizeof(command) - strlen(command) - 1);
        
        char* newline = strchr(command, '\n');
        if (newline) {
            *newline = '\0';
            log_message("Received FIFO command: '%s'", command);
            handle_command(command, config, OUTPUT_LOG);
            
            if (strlen(newline + 1) > 0) {
                memmove(command, newline + 1, strlen(newline + 1) + 1);
            } else {
                command[0] = '\0';
            }
        }
        
        if (strlen(command) >= sizeof(command) - 1) {
            log_message("Command buffer overflow, clearing");
            command[0] = '\0';
        }
    }
}

int interactive_mode(struct Daemon_cfg* config) {
    if (!config) return -1;

    log_message("Starting interactive mode for PID: %d", config->target_pid);
    printf("Run in interactive mode, PID: %d\n", config->target_pid);
    printf("Press 'q' to quit, 'help' for commands\n>>> ");
    fflush(stdout);

    set_nonblocking_mode(1);
    
    char cmd_buffer[256] = {0};
    int cmd_pos = 0;
    
    config->diff_history_count = 0;
    config->diff_history_start = 0;    
    config->last_snapshot = read_maps_snapshot(config->target_pid);
    if (!config->last_snapshot) {
        printf("ERROR: Failed to read initial snapshot\n");
        set_nonblocking_mode(0);
        return -1;
    }
    save_full_backup(config->last_snapshot, config->target_pid);

    unsigned long last_check = get_current_time_ms();
    
    while (config->is_running) {
        // Обработка ввода
        if (ready_to_read()) {
            char ch = getchar();
            
            if (ch == '\n' || ch == '\r') {
                if (cmd_pos > 0) {
                    cmd_buffer[cmd_pos] = '\0';
                    handle_command(cmd_buffer, config, OUTPUT_CONSOLE);
                    cmd_pos = 0;
                    memset(cmd_buffer, 0, sizeof(cmd_buffer));
                }
            } else if (ch == 127 || ch == 8) { 
                if (cmd_pos > 0) {
                    cmd_pos--;
                    printf("\b \b");
                    fflush(stdout);
                }
            } else if (cmd_pos < (int)sizeof(cmd_buffer) - 1) {
                cmd_buffer[cmd_pos++] = ch;
                putchar(ch);
                fflush(stdout);
            }
        }
        
        unsigned long current_time = get_current_time_ms();
        if (current_time - last_check >= (config->sample_interval / 1000)) {
            if (process_snapshot(config) == 0) {
                log_changes(&(struct Maps_diff){0}, 1);
            }
            last_check = current_time;
        }
        
        usleep(10000);
    }
    
    if (config->last_snapshot) {
        free_maps_snapshot(config->last_snapshot);
        config->last_snapshot = NULL;
    }
    
    set_nonblocking_mode(0);
    log_message("Exiting interactive mode");
    printf("\nExiting interactive mode\n");
    return 0;
}

int daemon_mode(struct Daemon_cfg* config) {
    if (!config) return -1;

    log_message("Starting daemon mode for PID: %d", config->target_pid);

    if (!check_target_process(config->target_pid)) {
        log_message("ERROR: Target process %d does not exist", config->target_pid);
        return -1;
    }
    
    config->diff_history_count = 0;
    config->diff_history_start = 0;    
    global_daemon_cfg = config;
    setup_signal_handlers();
    
    const char* fifo_path = "logs/daemon_fifo";
    mkfifo(fifo_path, 0666);
    
    pid_t child_pid = daemon_init();
    if (child_pid > 0) {
        printf("Daemon started with PID: %d\n", child_pid);
        printf("Use 'kill %d' to stop the daemon\n", child_pid);
        printf("Use 'kill -USR1 %d' to create manual backup\n", child_pid);
        printf("Use 'echo \"command\" > logs/daemon_fifo' to send commands\n");
        return 0;
    }
    
    log_message("Daemon process initialized");

    int fifo_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        log_message("ERROR: Failed to open FIFO");
    }

    config->last_snapshot = read_maps_snapshot(config->target_pid);
    if (!config->last_snapshot) {
        log_message("ERROR: Failed to read initial snapshot");
        if (fifo_fd != -1) close(fifo_fd);
        return -1;
    }
    save_full_backup(config->last_snapshot, config->target_pid);
    log_message("Initial backup created");

    unsigned long last_check = get_current_time_ms();
    int consecutive_errors = 0;
    const int MAX_CONSECUTIVE_ERRORS = 5;
    
    while (config->is_running) {
        unsigned long current_time = get_current_time_ms();
        
        if (fifo_fd != -1) {
            check_fifo_commands(fifo_fd, config);
        }
        
        if (current_time - last_check >= (config->sample_interval / 1000)) {
            if (process_snapshot(config) == 0) {
                log_changes(&(struct Maps_diff){0}, 0);
                consecutive_errors = 0;
            } else {
                consecutive_errors++;
                log_message("Snapshot error %d/%d", consecutive_errors, MAX_CONSECUTIVE_ERRORS);
                
                if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
                    log_message("Too many errors, shutting down");
                    config->is_running = 0;
                    break;
                }
            }
            last_check = current_time;
        }
        
        usleep(100000);
        
        static unsigned long last_process_check = 0;
        if (current_time - last_process_check >= 5000) {
            if (!check_target_process(config->target_pid)) {
                log_message("Target process %d terminated", config->target_pid);
                config->is_running = 0;
                break;
            }
            last_process_check = current_time;
        }
    }
    
    if (fifo_fd != -1) {
        close(fifo_fd);
        unlink(fifo_path);
    }
    
    if (config->last_snapshot) {
        free_maps_snapshot(config->last_snapshot);
        config->last_snapshot = NULL;
    }
    
    log_message("Daemon mode exiting gracefully");
    global_daemon_cfg = NULL;
    return 0;
}
