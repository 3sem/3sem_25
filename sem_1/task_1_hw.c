#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const int MAX_NUM_OF_TOKENS = 1048576;
const int MAX_STR_LEN = 1048576;
const int MAX_OF_CMD = 1048576;

char** parse_cmd(char* cmd) {
    char** tokens = (char**)calloc(MAX_NUM_OF_TOKENS, sizeof(char*));
    if (!tokens) {
        perror("calloc");
        exit(1);
    }

    const char* delim = " ";
    int i = 0;
    for (char* arg = strtok(cmd, delim); arg != NULL; arg = strtok(NULL, delim)) {
        tokens[i] = strdup(arg);
        if (!tokens[i]) {
            perror("strdup");
            exit(1);
        }
        i++;
    }
    tokens[i] = NULL;
    return tokens;
}

char*** parse_cmdline(char* cmdline) {
    char*** cmd_array = (char***)calloc(MAX_OF_CMD, sizeof(char**));
    if (!cmd_array) {
        perror("calloc");
        exit(1);
    }

    int num_of_cmd = 0;
    char* saveptr = NULL;
    for (char* part = strtok_r(cmdline, "|", &saveptr);
         part != NULL;
         part = strtok_r(NULL, "|", &saveptr))
    {
        while (*part == ' ') part++;
        char* end = part + strlen(part) - 1;
        while (end > part && *end == ' ') {
            *end = '\0';
            end--;
        }

        cmd_array[num_of_cmd] = parse_cmd(part);
        num_of_cmd++;
    }

    cmd_array[num_of_cmd] = NULL;
    return cmd_array;
}

void seq_pipe(char*** cmd)
{
    int   p[2];
    pid_t pid;
    int   fd_in = 0;
    int   i = 0;

    while (cmd[i] != NULL) {
            pipe(p);
           // printf("New used pipe descriptors: %d %d\n", p[0],p[1]);
           // printf("Input descriptor for current child process: %d\n", fd_in); удобно для последующего дебага
            if ((pid = fork()) == -1) {
                exit(1);
            } else if (pid == 0) {
                if (i > 0)
                    dup2(fd_in, 0); //stdin <- read from fd_in
                if (cmd[i+1] != NULL)
                   dup2(p[1], 1); //stdout -> write to pipe
                close(p[0]);
                execvp((cmd)[i][0], cmd[i]);
                exit(2);
            }
	    else {
		 int status;
                 if (waitpid(pid, &status, 0) == -1) {
                    perror("waitpid");
                    exit(1);
                }
                close(p[1]);
                if (i>0)
                    close(fd_in); // старый пайп больше не нужен
                fd_in = p[0]; // следующий процесс читает из этого конца
                i++;
            }
        }
    return;
}


int main() {
	while (1) {
		char str[MAX_STR_LEN];
        	printf(">> ");
		fgets(str, MAX_NUM_OF_TOKENS, stdin);
        	str[strcspn(str, "\n")] = '\0';
		seq_pipe(parse_cmdline(str));
	}
}

