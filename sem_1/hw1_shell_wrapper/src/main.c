#include "shell_wrapper.h"

int main() {
    char cmd_buf[MAX_CMD_SIZE] = {};
    char ***cmd = NULL;
    printf (YELLOW "This is a shell-wrapper emulator. To finish, enter \"exit\". Please include spaces on either side of the \"|\" separator." WHITE "\n");
    while (1)
    {
        printf ("> ");
        fgets (cmd_buf, MAX_CMD_SIZE, stdin);

        cmd_buf[strcspn(cmd_buf, "\n")] = '\0';
        if (strcmp (cmd_buf, "exit") == 0)
        {
            break;
        }
        if (strcmp (cmd_buf, "\n") == 0)
        {
            continue;
        }

        cmd = parsing_cmd (cmd_buf);
        if (!cmd)
        {
            continue;
        }
        run_cmd (cmd);

        cmd_list_dtor (cmd);
    }
    printf (YELLOW "Emulation completed." WHITE "\n");
    return 0;
}