#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

const char* CMD_SEP_WORD = "|";
const char* ARGS_SEP_WORD = " ";
const size_t MAX_INPUT = 4096;
const size_t MAX_CMDS_AMT = 20;
const size_t MAX_ARGS_AMT = 20;

struct CmdStruct
{
    char** cmd;                                     //команда с аргументами
    int exit_code;
};

//----------------------------------------------------------------------

void seq_pipe(CmdStruct* cmd);

CmdStruct* cmds_ctor();
CmdStruct* get_cmds();
void cmds_dtor(CmdStruct* cmds);

//----------------------------------------------------------------------

int main()
{
    while (1)
    {
        CmdStruct* cmds = get_cmds();

        seq_pipe(cmds);

        cmds_dtor(cmds);
    }

    return 0;
}

CmdStruct* cmds_ctor()
{
    CmdStruct* cmds = (CmdStruct*)calloc(MAX_CMDS_AMT, sizeof(CmdStruct));
    for (size_t i = 0; i < MAX_CMDS_AMT; i++)
    {
        char** args = (char**)calloc(MAX_ARGS_AMT, sizeof(char*));
        cmds[i].cmd = args;
        cmds[i].exit_code = 0;
    }

    return cmds;
}

void cmds_dtor(CmdStruct* cmds)
{
    int i = 0;
    int j = 0;

    while (cmds[i].cmd[0] != NULL && i < MAX_CMDS_AMT)
    {
        while (cmds[i].cmd[j] != NULL && j < MAX_ARGS_AMT)
        {
            free(cmds[i].cmd[j]);
            j++;
        }
        j = 0;
        i++;
    }

    free(cmds);
}

CmdStruct* get_cmds()
{
    char cmds_line[MAX_INPUT] = {};
    char cmd_buf[MAX_INPUT] = {};
    char word_buf[MAX_INPUT] = {};

    printf("Еnter the command\n");

    fflush(stdout);
    char* res = fgets(cmds_line, MAX_INPUT, stdin);
    if (res == NULL || !strlen(cmds_line))
        return NULL;
    
    cmds_line[strcspn(cmds_line, "\n")] = '\0';

    CmdStruct* cmds = cmds_ctor();

    int i = 0;
    int sscanf_res = 1;
    int cmd_chars_cnt = 0;

    while (i < MAX_CMDS_AMT)
    {
        sscanf_res = sscanf(cmds_line + cmd_chars_cnt, "%[^|]", cmd_buf);           //вернет строку с отдельной командой

        if (sscanf_res < 1)
            break;

        int j = 0;
        int words_chars_cnt = 0;
        while (j < MAX_ARGS_AMT)
        {
            int ind = 0;
            while (isspace(cmd_buf[ind]))                                           //пропускаем пробелы в начале
                ind++;

            sscanf_res = sscanf(cmd_buf + ind + words_chars_cnt, "%[^ ]", word_buf);
            if (sscanf_res < 1)                                                     //обрабатываем остаток строки
                break;

            cmds[i].cmd[j] = strdup(word_buf);

            j++;
            words_chars_cnt += strlen(word_buf) + 1;
        }
        cmds[i].cmd[j] = NULL;                                                      //метка, что аргументы в команде кончились
        i++;
        cmd_chars_cnt += strlen(cmd_buf) + 1;
    }
    cmds[i].cmd[0] = NULL;                                                          //метка, что команды закончились

    
    return cmds;
}

void seq_pipe(CmdStruct* cmd)
{
    int pipe_fds[2];
    pid_t pid;
    int fd_in = 0;
    size_t cmd_num = 0;
    int status = 0;

    while (cmd[cmd_num].cmd[0] != NULL) 
    {
        pipe(pipe_fds);
        if ((pid = fork()) == -1)
            exit(1);

        else if (!pid)
        {
            if (cmd_num > 0)
                dup2(fd_in, 0);

            if (cmd[cmd_num + 1].cmd[0] != NULL)                                //выведет результат всех команд в терминал пользователю, если команда последняя
                dup2(pipe_fds[1], 1);

            close(pipe_fds[0]);

            execvp(cmd[cmd_num].cmd[0], cmd[cmd_num].cmd);                      //cmd - массив строк до разделителя, первый элемент - типа путь
            exit(2);
        } 
        else 
        {
            waitpid(pid, &status, 0);
            cmd[cmd_num].exit_code = WEXITSTATUS(status);
            if (cmd[cmd_num].exit_code)
            {
                printf("There is an error! Cmd %s, exit code %d\n", cmd[cmd_num].cmd[0], cmd[cmd_num].exit_code);
                break;
            }

            close(pipe_fds[1]);

            if (cmd_num > 0)
                close(fd_in);

            fd_in = pipe_fds[0];
            cmd_num++;
        }
    }

    return;
}

