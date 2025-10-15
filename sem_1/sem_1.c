#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_COM  21

typedef struct
{
    char* args[MAX_ARGS];
    int argc;
} cmd_t;

char* safe_strdup(const char* src) {
    if (!src) return NULL;
    char* dst = malloc(strlen(src) + 1);
    if (dst) strcpy(dst, src);
    return dst;
}

int parse_line(char* line, cmd_t* commands)
{
    int cmd_count = 0;
    char* line_copy = safe_strdup(line);
    if (!line_copy) return 0;
    
    char* token = strtok(line_copy, "|");
    
    while (token != NULL && cmd_count < MAX_COM) 
    {
        while (*token == ' ') token++;
        
        char* end = token + strlen(token) - 1;
        while (end > token && *end == ' ') {
            *end = '\0';
            end--;
        }
        
        if (strlen(token) == 0) {
            token = strtok(NULL, "|");
            continue;
        }
        
        commands[cmd_count].argc = 0;
        
        char* cmd_copy = safe_strdup(token);
        if (!cmd_copy) {
            token = strtok(NULL, "|");
            continue;
        }
        
        char* arg = strtok(cmd_copy, " ");
        
        while (arg != NULL && commands[cmd_count].argc < MAX_ARGS - 1) 
        {
            commands[cmd_count].args[commands[cmd_count].argc++] = safe_strdup(arg);
            arg = strtok(NULL, " ");
        }
        commands[cmd_count].args[commands[cmd_count].argc] = NULL;
        
        free(cmd_copy);
        cmd_count++;
        token = strtok(NULL, "|");
    }
    
    free(line_copy);
    return cmd_count;
}

void free_commands(cmd_t* commands, int cmd_count)
{
    for (int i = 0; i < cmd_count; i++) {
        for (int j = 0; j < commands[i].argc; j++) {
            free(commands[i].args[j]);
        }
        commands[i].argc = 0;
    }
}

int execute_commands(cmd_t* commands, int cmd_count) 
{
    if (cmd_count <= 0) return -1;
    
    int prev_pipe[2] = {-1, -1};
    int next_pipe[2];
    pid_t pids[MAX_COM];
    int status;
    
    for (int i = 0; i < cmd_count; i++) 
    {
        if (i < cmd_count - 1) {
            if (pipe(next_pipe) == -1) {
                perror("pipe");
                return -1;
            }
        }
        
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return -1;
        }
        
        if (pid == 0) {
            if (i > 0) {
                dup2(prev_pipe[0], STDIN_FILENO);
            }
            
            if (i < cmd_count - 1) {
                dup2(next_pipe[1], STDOUT_FILENO);
            }
            
            if (i > 0) {
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }
            if (i < cmd_count - 1) {
                close(next_pipe[0]);
                close(next_pipe[1]);
            }
            
            execvp(commands[i].args[0], commands[i].args);
            perror("execvp");
            exit(127);
        } 
        else 
        {
            pids[i] = pid;
            
            if (i > 0) {
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }
            
            if (i < cmd_count - 1) {
                prev_pipe[0] = next_pipe[0];
                prev_pipe[1] = next_pipe[1];
            }
        }
    }
    
    if (cmd_count > 1) {
        close(prev_pipe[0]);
        close(prev_pipe[1]);
    }
    
    int exit_status = 0;
    for (int i = 0; i < cmd_count; i++) {
        waitpid(pids[i], &status, 0);
        if (i == cmd_count - 1) {
            if (WIFEXITED(status)) {
                exit_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                exit_status = 128 + WTERMSIG(status);
            }
        }
    }
    
    return exit_status;
}

int main() 
{
    char line[1024] = {0};
    cmd_t commands[MAX_COM];
    
    printf("This is emulator of terminal, write 'exit' for exit.\n");
    
    while (1) 
    {
        printf("> ");
        fflush(stdout);
        
        if (fgets(line, sizeof(line), stdin) == NULL) 
        {
            break;
        }
        
        line[strcspn(line, "\n")] = '\0';
        
        if (strcmp(line, "exit") == 0) 
        {
            break;
        }
        
        if (strlen(line) == 0) 
        {
            continue;
        }
        
        int cmd_count = parse_line(line, commands);
        
        if (cmd_count == 0) 
        {
            continue;
        }
        
        int status = execute_commands(commands, cmd_count);
        
        if (status != 0) 
        {
            fprintf(stderr, "Command exited with status %d\n", status);
        }
        
        free_commands(commands, cmd_count);
    }
    
    printf("Emulator finished\n");
    return 0;
}