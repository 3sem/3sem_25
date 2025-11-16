#include "monitor_modes.h"

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

void handle_interactive_command(const char* cmd, struct Daemon_cfg* config) {
    if (!cmd || !config) return;
    
    char clean_cmd[256];
    strncpy(clean_cmd, cmd, sizeof(clean_cmd) - 1);
    char* newline = strchr(clean_cmd, '\n');
    if (newline) *newline = '\0';
    
    printf("\n>>> Command: %s\n", clean_cmd);
    
    if (strcmp(clean_cmd, "q") == 0 || strcmp(clean_cmd, "quit") == 0) {
        printf("Exiting...\n");
        config->is_running = 0;
    }
    else if (strcmp(clean_cmd, "status") == 0) {
        printf("Current PID: %d\n", config->target_pid);
        printf("Sample interval: %d us\n", config->sample_interval);
        printf("Status: %s\n", config->is_running ? "running" : "stopped");
    }
    else if (strncmp(clean_cmd, "interval ", 9) == 0) {
        int new_interval = atoi(clean_cmd + 9);
        if (new_interval > 0) {
            config->sample_interval = new_interval;
            printf("Interval was changed to: %d мс\n", new_interval);
        }
    }
    else if (strncmp(clean_cmd, "pid ", 4) == 0) {
        pid_t new_pid = atoi(clean_cmd + 4);
        if (new_pid > 0) {
            config->target_pid = new_pid;
            printf("PID was changed to: %d\n", new_pid);
            if (config->last_snapshot) {
                free_maps_snapshot(config->last_snapshot);
            }
            config->last_snapshot = read_maps_snapshot(new_pid);
        }
    }
    else if (strcmp(clean_cmd, "help") == 0) {
        printf("\nAvailable commands:\n");
        printf("  status          - current status\n");
        printf("  interval <ms>   - change sample time\n");
        printf("  pid <new_pid>   - change target PID\n");
        printf("  help            - this reference\n");
        printf("  quit            - exit\n");
    }
    else {
        printf("Unknown: '%s'. insert 'help' for reference.\n", clean_cmd);
    }
    
    printf(">>> ");  
    fflush(stdout);
}

int interactive_mode(struct Daemon_cfg* config) {
    printf("run in interactive mode, PID: %d\n", config->target_pid);
    printf("Insert 'help' to list commands\n");
    printf(">>> ");
    fflush(stdout);

    set_nonblocking_mode(1);
    
    char cmd_buffer[256] = {0};
    int cmd_pos = 0;
    
    if (config->last_snapshot) {
        free_maps_snapshot(config->last_snapshot);
    }
    config->last_snapshot = read_maps_snapshot(config->target_pid);

    while (config->is_running) {
        if (ready_to_read()) {
            char ch = getchar();
            
            if (ch == '\n' || ch == '\r') {
                if (cmd_pos > 0) {
                    cmd_buffer[cmd_pos] = '\0';
                    handle_interactive_command(cmd_buffer, config);
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
        
        static unsigned long last_check = 0;
        unsigned long current_time = get_current_time_ms();
        
        if (current_time - last_check >= config->sample_interval) {
            struct Maps_snapshot* current = read_maps_snapshot(config->target_pid);
            if (current) {
                print_changes(config->last_snapshot, current);
                
                if (config->last_snapshot) {
                    free_maps_snapshot(config->last_snapshot);
                }
                config->last_snapshot = current;
                
                last_check = current_time;
            }
        }
        
        usleep(10000);
    }
    
    set_nonblocking_mode(0);
    printf("\nExiting interactive mode...\n");
    return 0;
}

void print_changes(struct Maps_snapshot* old, struct Maps_snapshot* new) {
    if (!old) {
        printf("\n[Инициализация] Загружено %zu mappings\n", 
               new->maps.size);
        return;
    }
    
    size_t old_count = old->maps.size;
    size_t new_count = new->maps.size;
    
    if (old_count != new_count) {
        printf("\n[Изменение] Количество mappings: %zu -> %zu\n", old_count, new_count);
    }
}
