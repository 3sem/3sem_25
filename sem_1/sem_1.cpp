#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

const int MAX_ARGS = 64;
const int MAX_COM  = 21;

typedef struct
{
    char* args[MAX_ARGS];
    int argc;
} cmd_t;

int parse_line(char* line, cmd_t* commands)
{
    int cmd_count = 0;
    char* token = strtok(line, "|");
    
    while (token != NULL) 
    {
        while (*token == ' ') token++;
        char* end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';
        
        commands[cmd_count].argc = 0;
        char* arg = strtok(token, " ");
        
        while (arg != NULL && commands[cmd_count].argc < MAX_ARGS - 1) 
        {
            commands[cmd_count].args[commands[cmd_count].argc++] = arg;
            arg = strtok(NULL, " ");
        }
        commands[cmd_count].args[commands[cmd_count].argc] = NULL;
        
        cmd_count++;
        token = strtok(NULL, "|");
    }
    
    return cmd_count;
}

int execute_commands(cmd_t* commands, int cmd_count) 
{
    int pipes[2][2];
    int input_fd = STDIN_FILENO;
    pid_t pids[cmd_count];
    int status;

    if (pipe(pipes[0]) == -1 || pipe(pipes[1]) == -1) 
    {
        perror("pipe");
        return -1;
    }

    int cur_pipe = 0;
    
    for (int i = 0; i < cmd_count; i++) 
    {
        if (i < cmd_count - 1) 
        {
            cur_pipe = 1 - cur_pipe;
            
            if (i > 0) 
            {
                close(pipes[1 - cur_pipe][1]);
            }
            
            if (i < cmd_count - 2 && pipe(pipes[cur_pipe]) == -1) 
            {
                perror("pipe");
                return -1;
            }
        }

        pid_t pid = fork();
        if (pid == -1) 
        {
            perror("fork");
            return -1;
        }
        
        if (pid == 0) 
        { 
            if (input_fd != STDIN_FILENO) 
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            
            if (i < cmd_count - 1) 
            {
                close(pipes[cur_pipe][0]); 
                dup2(pipes[cur_pipe][1], STDOUT_FILENO);
                close(pipes[cur_pipe][1]);
            }
            
            for (int j = 0; j < 2; j++) 
            {
                for (int k = 0; k < 2; k++) 
                {
                    if (pipes[j][k] != STDIN_FILENO && pipes[j][k] != STDOUT_FILENO) 
                    {
                        close(pipes[j][k]);
                    }
                }
            }
            
            execvp(commands[i].args[0], commands[i].args);
            perror("execvp");
            exit(-1);
        } 
        else 
        {  
            pids[i] = pid;
            
            if (input_fd != STDIN_FILENO) 
            {
                close(input_fd);
            }
            
            if (i < cmd_count - 1) 
            {
                close(pipes[cur_pipe][1]); 
                input_fd = pipes[cur_pipe][0];  
            }
        }
    }
    
    for (int i = 0; i < 2; i++) 
    {
        for (int j = 0; j < 2; j++) 
        {
            if (pipes[i][j] != STDIN_FILENO && pipes[i][j] != STDOUT_FILENO) 
            {
                close(pipes[i][j]);
            }
        }
    }
    
    int exit_status = 0;
    for (int i = 0; i < cmd_count; i++) 
    {
        waitpid(pids[i], &status, 0);
        
        if (i == cmd_count - 1) 
        {
            if (WIFEXITED(status)) 
            {
                exit_status = WEXITSTATUS(status);
            } 
            else if (WIFSIGNALED(status)) 
            {
                exit_status = -1;
            }
        }
    }
    
    return exit_status;
}


int main() 
{
    char line[1024]= {0};
    cmd_t commands[MAX_COM];
    
    printf("This is emulator of terminal, write 'exit' for exit.\n");
    
    while (1) 
    {
        printf("> ");
        
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
        
        if (status == -1) 
        {
            fprintf(stderr, "command execute error\n");
        }
    }
    
    printf("Emulator finished\n");
    return 0;
}