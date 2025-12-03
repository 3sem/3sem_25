#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>

#define MAX_ARGS 128
#define MAX_CMDS 32


void parseCmd (char *cmd, char **argv) {
    size_t argc = 0;
    char *token = strtok(cmd, " \t");
    while (token && argc < 127) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    argv[argc] = NULL;
}

int parsePipeline (char *line, char **cmds) {
    int ncmds = 0;
    char *token = strtok(line, "|");
    while (token && ncmds < MAX_CMDS) {
        while (*token == ' ' || *token == '\t') token++;
        cmds[ncmds++] = token;
        token = strtok(NULL, "|");
    }

    return ncmds;
}

void singleComandExe (char *cmd) {
    char *argv[MAX_ARGS];
    parseCmd(cmd, argv);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-42);
    }

    if (pid == 0) {             // Child
        execvp(argv[0], argv);
        perror("Execute ERROR");
        exit(42);
    } else {                    // Parent
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("\n>>>> Process %d exited with exit code %d\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("\n>>>> Process %d killed by signal %d\n", pid, WTERMSIG(status));
        }
    }
    
}

void pipelineExe (char *buf) {
    char *cmds[MAX_CMDS];
    int ncmds = parsePipeline(buf, cmds);

    int pipes[MAX_CMDS - 1][2];
    for (int i = 0; i < ncmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return;
        }
    }

    pid_t pids[MAX_CMDS];

    for (int i = 0; i < ncmds; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }

        if (pid == 0) {  // Child
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            if (i < ncmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < ncmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            char *argv[MAX_ARGS];
            parseCmd(cmds[i], argv);
            execvp(argv[0], argv);
            perror("execvp");
            exit(42);
        }

        pids[i] = pid;
    }


    // Parent
    for (int i = 0; i < ncmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int exStatuses[MAX_CMDS];
    for (int i = 0; i < ncmds; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        exStatuses[i] = status;
    }
    
    for (int i = 0; i < ncmds; i++) {
        if (WIFEXITED(exStatuses[i])) {
            printf(">>>> Process %d exited with %d\n", pids[i], WEXITSTATUS(exStatuses[i]));
        } else if (WIFSIGNALED(exStatuses[i])) {
            printf(">>>> Process %d killed by signal %d\n", pids[i], WTERMSIG(exStatuses[i]));
        }
    }
    


}

int main(void) {
    char buf[4096];

    while (1) {
        printf("my-shell$ ");
        fflush(stdout);

        if (!fgets(buf, sizeof(buf), stdin)) {
            break;
        }
        buf[strcspn(buf, "\n")] = '\0';
        if (strcmp(buf, "exit") == 0) {
            break;
        }

        if (strchr(buf, '|')) {
            pipelineExe(buf);
        } else {
            singleComandExe(buf);
        }
    }
        

    printf("Exit my-shell\n");
    return 0;
}
