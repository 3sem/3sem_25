#include <stdio.h>
#include <stdlib.h>

#include "cmd.h"

int main() {
    char cmd_str_buf[CMD_BUFF_SIZE] = {};
    cmd_big_t * list;

    while(scanf("%[^\n]", cmd_str_buf) == 1) {
        getchar();
        if (cmd_str_buf[sizeof(cmd_str_buf) - 1]) {
            perror("too long command\n");
            exit(1);
        }

        if (!(list = str_to_cmd_big(cmd_str_buf))) {
            perror("make command sequence from string failed\n");
            exit(1);
        }

        if (run_cmd_big(list)) {
            perror("run command sequence failed\n");
        }

        printf("______________________________________\n\n");
    }

    dtor_cmd_big(list);

    return 0;
}
