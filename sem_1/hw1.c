/*Write a 'shell-wrapper' program which allow users to:

    run programs via command line cyclically getting commands from STDIN and running it somewhere, e.g. in child process.
    get exit codes of terminated programs
    use pipelining of program sequences, for ex.: «env | grep HOSTNAME | wc»
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

typedef struct {
    char **argv;
} command_t;

typedef struct {
    command_t *cmds;
    int count;
} pipeline_t;

pipeline_t* parse_input(char *line) {
    int bufsize = 64;
    int pos = 0;
    char **pipes = malloc(bufsize * sizeof(char*));
    char *token;
    char *saveptr1;

    if (!pipes) exit(EXIT_FAILURE);

    token = strtok_r(line, "|", &saveptr1);
    while (token != NULL) {
        pipes[pos++] = token;
        if (pos >= bufsize) {
            bufsize += 64;
            pipes = realloc(pipes, bufsize * sizeof(char*));
            if (!pipes) exit(EXIT_FAILURE);
        }
        token = strtok_r(NULL, "|", &saveptr1);
    }
    pipes[pos] = NULL;

    pipeline_t *pipeline = malloc(sizeof(pipeline_t));
    pipeline->count = pos;
    pipeline->cmds = malloc(pos * sizeof(command_t));

    for (int i = 0; i < pos; i++) {
        int arg_bufsize = 64;
        int arg_pos = 0;
        char **args = malloc(arg_bufsize * sizeof(char*));
        char *arg_token;
        char *saveptr2;

        if (!args) exit(EXIT_FAILURE);

        arg_token = strtok_r(pipes[i], " \t\r\n\a", &saveptr2);
        while (arg_token != NULL) {
            args[arg_pos++] = arg_token;
            if (arg_pos >= arg_bufsize) {
                arg_bufsize += 64;
                args = realloc(args, arg_bufsize * sizeof(char*));
                if (!args) exit(EXIT_FAILURE);
            }
            arg_token = strtok_r(NULL, " \t\r\n\a", &saveptr2);
        }
        args[arg_pos] = NULL;
        pipeline->cmds[i].argv = args;
    }

    free(pipes);
    return pipeline;
}

void free_pipeline(pipeline_t *pipeline) {
    if (!pipeline) return;
    for (int i = 0; i < pipeline->count; i++) {
        free(pipeline->cmds[i].argv);
    }
    free(pipeline->cmds);
    free(pipeline);
}

void execute_pipeline(pipeline_t *pipeline) {
    int i;
    int fd[2];
    int prev_read_fd = -1; 
    pid_t pid;

    for (i = 0; i < pipeline->count; i++) {
        if (pipeline->cmds[i].argv[0] == NULL) continue;

        if (i < pipeline->count - 1) {
            if (pipe(fd) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            
            if (prev_read_fd != -1) {
                dup2(prev_read_fd, STDIN_FILENO);
                close(prev_read_fd);
            }

            if (i < pipeline->count - 1) {
                close(fd[0]); 
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }

            if (execvp(pipeline->cmds[i].argv[0], pipeline->cmds[i].argv) < 0) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } else {
            
            if (prev_read_fd != -1) {
                close(prev_read_fd);
            }

            if (i < pipeline->count - 1) {
                close(fd[1]);
                prev_read_fd = fd[0];
            }
        }
    }

    while (wait(NULL) > 0);
}

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while (1) {
        printf("> ");
        nread = getline(&line, &len, stdin);
        
        if (nread == -1) {
            break;
        }

        pipeline_t *pipeline = parse_input(line);
        if (pipeline->count > 0) {
            execute_pipeline(pipeline);
        }
        free_pipeline(pipeline);
    }
    
    if (line) free(line);
    return 0;
} 

