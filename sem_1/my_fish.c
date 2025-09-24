#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

static const int initialCharsInCmd = 256;
static const int initialLexems     = 8;

typedef struct Status {
    int exited;
    int exitCode;
    int last;
} Status;

static void processStatus (Status * s) {
    int needToPrint = 0;
    for (int i = 0; s[i].last != 1; i++) {
        if (!s[i].exited || (s[i].exited && s[i].exitCode)) needToPrint = 1;
    }

    if (needToPrint) {
        printf("\E[31;40m[");
        for (int i = 0; s[i].last != 1; i++) {
            if (s[i+1].last != 1) {
                if (!s[i].exited) printf("x|");
                else              printf("%d|", s[i].exitCode);
            } else {
                if (!s[i].exited) printf("x");
                else              printf("%d", s[i].exitCode);
            }
        }
        printf("]\E[37;40m");
    }
}

static size_t countNumOfPipes(char * cmd) {
    size_t num = 1;
    for (int i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == '|') {
            num++;
            cmd[i] = '\0';
        }
    }

    return num;
}

static char ** parseOneCmd(char * cmd) {
    char *p;
    char delim[] = " \n";

    char ** args      = calloc(initialLexems, sizeof(char *));
    int     numOfArgs = initialLexems;
    int     counter   = 0;

    if (!args) {
        fprintf(stderr, "Cannot allocate memory for args in one of cmd\n");
        exit(1);
    }

    char * copyCmd = strdup(cmd);

    for (char *p = strtok(copyCmd, delim); p != NULL; p = strtok(NULL, delim)) {
        args[counter] = strdup(p);
        counter++;

        if (numOfArgs == counter) {
            args = realloc(args, sizeof(char *) * numOfArgs * 2);
            numOfArgs *= 2;

            if (!args) {
                fprintf(stderr, "Cannot reallocate memory for args in one of cmd\n");
                exit(1);
            }
        }
    }

    free(copyCmd);
    args[numOfArgs] = NULL;
    return args;
}

static char *** partCmd(char * cmd, Status ** s) {
    size_t   numOfCmds = countNumOfPipes(cmd);
    char *** partedCmd = calloc(numOfCmds + 1, sizeof(char **));
    *s = calloc(numOfCmds + 1, sizeof(Status));

    if (!partedCmd) {
        fprintf(stderr, "Cannot allocate memory for parted cmd\n");
        exit(1);
    }

    int counterOfProcessedCmds = 0;
    while (counterOfProcessedCmds != numOfCmds) {
        partedCmd[counterOfProcessedCmds] = parseOneCmd(cmd);
        for (; *cmd != '\0'; cmd++);
        cmd++;
        counterOfProcessedCmds++;
    }

    partedCmd[counterOfProcessedCmds] = NULL;
    (*s)[counterOfProcessedCmds] = (Status) {0, 0, 1};

    return partedCmd;
}

static void freeLexems(char *** partedCmd) {
    for (int i = 0; partedCmd[i] != NULL; i++) {
        for (int j = 0; partedCmd[i][j] != NULL; j++) {
            free(partedCmd[i][j]);
        }
        free(partedCmd[i]);
    }

    free(partedCmd);
}

void execInTerminal(char ***cmd, Status * s)
{
  int   p[2];
  pid_t pid;
  int   fd_in = 0;
  int   i = 0;
  int   status = 0;

  while (cmd[i] != NULL) {
    pipe(p);
    if ((pid = fork()) == -1) {
          exit(1);
    } else if (pid == 0) {
        if (i > 0)
          dup2(fd_in, 0);
        if (cmd[i+1] != NULL)
          dup2(p[1], 1);
        close(p[0]);
        execvp((cmd)[i][0], cmd[i]);
        exit(2);
    } else {
      waitpid(pid, &status, 0);
      s[i].exited = WIFEXITED(status);
      s[i].exitCode = WEXITSTATUS(status);
      close(p[1]);
      if (i>0)
        close(fd_in);
      fd_in = p[0];
      i++;
    }
}
  return;
}

int main() {
    Status * s = 0;
    while (1) {
        printf("> ");

        char * allCmd    = NULL;
        size_t sizeOfCmd = initialCharsInCmd;
        getline(&allCmd, &sizeOfCmd, stdin);

        char *** partedCmd = partCmd(allCmd, &s);

        execInTerminal(partedCmd, s);

        freeLexems(partedCmd);
        free(allCmd);
        processStatus(s);
    }
}