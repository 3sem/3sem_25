#include "shell_wrapper.h"

void cmd_list_dtor (char ***cmd_list)
{
    for (int i = 0; cmd_list[i] != NULL; i++)
    {
        for (int j = 0; cmd_list[i][j] != NULL; j++)
        {
            free (cmd_list[i][j]);
        }
        free (cmd_list[i]);
    }
    free (cmd_list);
}

char ***parsing_cmd (char *cmd_buf)
{
    char ***cmd_list = (char ***)calloc (MAX_CMD_COUNT, sizeof(char **));
    if (!cmd_list) {
        printf (RED "Calloc error." WHITE "\n");
        return NULL;
    }
    int cmd_num = 0;
    int arg_num = 0;
    cmd_list[cmd_num] = (char **)calloc (MAX_ARG_COUNT, sizeof(char*));
    if (!cmd_list[cmd_num])
    {
        printf (RED "Calloc error." WHITE "\n");
        return NULL;
    }

    for (char *token = strtok(cmd_buf, " "); token != NULL; token = strtok(NULL, " "))
    {
        if (strcmp (token, "|") == 0)
        {
            cmd_num++;
            arg_num = 0;
            cmd_list[cmd_num] = (char **)calloc (MAX_ARG_COUNT, sizeof(char*));
            if (!cmd_list[cmd_num])
            {
                printf (RED "Calloc error." WHITE "\n");
                return NULL;
            }
            continue;
        }

        if (arg_num >= MAX_ARG_COUNT)
        {
            printf (RED "Exceeding the arguments limit." WHITE "\n");
            return NULL;
        }

        if (cmd_num >= MAX_CMD_COUNT)
        {
            printf (RED "Exceeding the command limit." WHITE "\n");
            return NULL;
        }

        cmd_list[cmd_num][arg_num] = (char *)calloc (MAX_CMD_SIZE, sizeof(char));
        if (!cmd_list[cmd_num][arg_num])
        {
            printf (RED "Calloc error." WHITE "\n");
            return NULL;
        }

        strcpy(cmd_list[cmd_num][arg_num], token);  
        arg_num++;
    }
    
    return cmd_list;
}


void run_cmd(char ***cmd) 
{
    int   p[2];
    pid_t pid;
    int   fd_in = 0;
    int   i = 0;

    while (cmd[i] != NULL) 
    {
        pipe(p);
        if ((pid = fork()) == -1) 
        {
                printf (RED "Fork error." WHITE "\n");
                return;
        } 
        else if (pid == 0) 
        {
            if (i > 0)
              dup2(fd_in, 0); 
            if (cmd[i+1] != NULL)
              dup2(p[1], 1); 
            close(p[0]);
            execvp((cmd)[i][0], cmd[i]);
            exit(2);
        } 
        else 
        {
            int status;
		    waitpid(pid, &status, 0);
		    printf(BLUE "Ret code: %d" WHITE "\n", WEXITSTATUS(status));
            close(p[1]);
            if (i>0)
                close(fd_in); 
            fd_in = p[0]; 
            i++;
        }
    }
}

