#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_SUBSTRINGS   32
#define INPUT_CHARS    1024

const char arg_delim[] = " \n";
const char cmd_delim[] = "|";

char **parse(const char *string, const char *delim) {
    char **substrings = (char **) calloc(MAX_SUBSTRINGS, sizeof(char *));
    char *string_copy = strdup(string);

    int i = 0;
    for (char *p = strtok(string_copy, delim); p != NULL; p = strtok(NULL, delim)) {
        substrings[i] = (char *) malloc(sizeof(char) * (strlen(p) + 1));
        strcpy(substrings[i++], p);
    }

    free(string_copy);
    substrings[i] = NULL;
    return substrings;
}

static void run(const char *input) {
    char **commands = parse(input, cmd_delim);
    int fd_in = 0;

    for (int i = 0; commands[i] != NULL; i++) {
        char *cmd = commands[i];
        int fd[2];

        if (pipe(fd) < 0) {
            printf("Pipe creation failed!\n");
            return;
        }

        const pid_t pid = fork();

        if (pid < 0) {
            printf("Fork failed!\n");
            return;
        }

        if (pid) {
            close(fd[1]);

            int status;
            waitpid(pid, &status, 0);

            if (WEXITSTATUS(status))
                printf("Exit status %d\n", WEXITSTATUS(status));

            if (i > 0)
                close(fd_in);

            fd_in = fd[0];
        } else {
            if (i > 0) {
                dup2(fd_in, 0);
                close(fd_in);
            }

            if (commands[i + 1] != NULL) {
                dup2(fd[1], 1);
                close(fd[1]);
                close(fd[0]);
            }

            char **args = parse(cmd, arg_delim);
            execvp(args[0], args);
            printf("Execvp failed!\n");

            for (int j = 0; args[j] != NULL; j++) {
                free(args[j]);
            }
            free(args);
        }
    }

    for (int j = 0; commands[j] != NULL; j++) {
        free(commands[j]);
    }
    free(commands);
}

int main() {
    while(1) {
        char *input = (char *) calloc(INPUT_CHARS, sizeof(char));
        fgets(input, INPUT_CHARS, stdin);
        run(input);
        free(input);
    }

    return 0;
}