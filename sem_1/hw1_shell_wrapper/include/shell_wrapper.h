#include <unistd.h>
#include <sys/syscall.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define RED "\033[1;31m"
#define WHITE "\033[0m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAX_CMD_SIZE 100
#define MAX_CMD_COUNT 10
#define MAX_ARG_COUNT 10

void cmd_list_dtor (char ***cmd_list);
char ***parsing_cmd (char *cmd_buf);
void run_cmd(char ***cmd);