#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cmd.h"
#include "common.h"

int get_cmd_num(const char * str) {
    if (!str) return -1;

    int cnt = 1;
    for (int i = 0; str[i]; cnt += (str[i++] == SPLIT_CMD_CHAR ? 1 : 0));
    return cnt;
}
int get_cmd_arg_num(const char * str) {
    if (!str) return -1;

    int cnt = 0;
    int prev_is_space = 1;

    while (!*str) {
        if (*(str++) == SPLIT_ARG_CHAR) {
            prev_is_space = 1;
        } else if (prev_is_space) {
            cnt++;
            prev_is_space = 0;
        }
    }

    return cnt;
}

int dtor_cmd_big(cmd_big_t * list) {
    if (!list) return 0;

    if (list->cmd) {
        for(int i = 0; i < list->cmd_num; ++i)  {
            if (list->cmd[i].argv) free(list->cmd[i].argv);
            SET_ZERO((list->cmd + i));
        }
        free(list->cmd);
    }
    if (list->str) free(list->str);

    SET_ZERO(list);
    free(list);

    return 0;
}
cmd_big_t * ctor_cmd_big(int cmd_num, int cmd_str_size) {
    cmd_big_t * list = 0;
    CALLOC_ERR(list, 1, 0);

    list->cmd_num = cmd_num;
    CALLOC_CTOR(list->cmd, cmd_num     , list, dtor_cmd_big, 0);
    CALLOC_CTOR(list->str, cmd_str_size, list, dtor_cmd_big, 0);

    return list;
}

int split_cmd(cmd_t * cmd) {
    if (!cmd) return 1;

    int argc = get_cmd_arg_num(cmd->start) + 1;
    CALLOC_ERR(cmd->argv, argc, 1);

    char * str = cmd->start - 1;
    int prev_is_space = 1;
    char ** curr_arg = cmd->argv;

    while (*(++str)) {
        if (*str == SPLIT_ARG_CHAR) {
            prev_is_space = 1;
            *str = 0;
        } else if (prev_is_space) {
            prev_is_space = 0;
            *(curr_arg++) = str;
        }
    }

    return 0;
}
int split_cmd_big(cmd_big_t * list) {
    if (!list) return 1;

    char * cmd_r = list->str;
    char * cmd_l = list->str;
    cmd_t * curr_cmd = list->cmd;

    while (*(++cmd_r)) {
        if (*cmd_r == SPLIT_CMD_CHAR) {
            *cmd_r = '\0';
            curr_cmd->start = cmd_l;
            cmd_l = cmd_r + 1;
            if (split_cmd(curr_cmd++)) {
                return 1;
            }
        }
    }

    return 0;
}

cmd_big_t * str_to_cmd_big(const char * cmd_str) {
    if (!cmd_str) return 0;

    int str_size = strlen(cmd_str) + 1;
    cmd_big_t * list = ctor_cmd_big(get_cmd_num(cmd_str), str_size);
    if(!list) {
        perror("create list failed\n");
        return 0;
    }

    strcpy(list->str, cmd_str);
    list->str[str_size - 1] = SPLIT_CMD_CHAR;

    if (split_cmd_big(list)) {
        dtor_cmd_big(list);
        return 0;
    }

    return list;
}

int run_cmd(cmd_t * cmd, int fd_in, int fd_out) {
    dup2(fd_in,  STDIN_FILENO);
    dup2(fd_out, STDOUT_FILENO);

    execvp(*(cmd->argv), cmd->argv);

    perror("execvp failed\n");
    return 1;
}

int run_cmd_big(cmd_big_t * list) {
    pid_t pid = 0;
    int status = 0;

    int pipefd[2] = {};
    int fd_in  = 0;
    int fd_out = 0;

    printf("run command sequence (size = %d)\n", list->cmd_num);

    for (int i = 0; i < list->cmd_num; ++i) {
        if (pipe(pipefd) == -1) {
            perror("pipe failed/n");
            return 1;
        }

        fd_out = pipefd[1];

        if ((pid = fork()) == -1) {
            perror("fork failed/n");
            return 1;
        } else if (pid == 0) {
            close(pipefd[0]);

            if (run_cmd(list->cmd + i, fd_in, fd_out)) {
                perror("run command failed\n");
                return 1;
            }
        }
        waitpid(pid, &status, 0);

        close(fd_out);
        if (i) close(fd_in);
        fd_in = pipefd[0];

        printf("command %d exit code: %d\n", i, status);
    }

    printf("No errors occured\n\n");

    if (write_result(fd_in)) {
        perror("write result failed\n");
        return 1;
    }
    close (fd_in);
    return 0;
}
