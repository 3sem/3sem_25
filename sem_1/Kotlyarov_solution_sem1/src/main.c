#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "parse_cmd.h"

int main() {

    char* cmd_buf = NULL, * cmd_buf_cpy = NULL;
    size_t cmd_buf_size = 0;
    char** cmd_buf_ptr = &cmd_buf;
    ssize_t read = getline(&cmd_buf, &cmd_buf_size, stdin);
    cmd_buf_cpy = cmd_buf;  
    if (read <= 0) {
        DEBUG_PRINTF("ERROR or EOF\n");
        return 1;
    }

    while (read > 0 && (cmd_buf[read - 1] == '\n' || cmd_buf[read - 1] == '\r')) {
        cmd_buf[--read] = '\0';
    }

    int prev_read_fd = -1;
    Dynamic_array dicts_d_array = {};
    Dynamic_array* dicts_d_array_ptr = &dicts_d_array;
    if (!Dynamic_array_ctor(dicts_d_array_ptr, D_array_init_size, 0))
        return 1;

    cmd_args_dict* dict = parse_cmd(cmd_buf, cmd_buf_ptr);
    while (dict) {
        D_ARRAY_PUSH_PTR(dicts_d_array_ptr, dict);
        dict = parse_cmd(NULL, cmd_buf_ptr);
    }
    
    cmd_args_dict** dicts = (cmd_args_dict**) dicts_d_array.data; 
    size_t chld_proc_amount = dicts_d_array_ptr->size / sizeof(cmd_args_dict*);
    if (chld_proc_amount == 0) {
        return 0;
    }

    for (size_t i = 0; i < chld_proc_amount; i++) {

        int pipefd[2] = {-1, -1};
        
        if (i < chld_proc_amount - 1) {
            if (pipe(pipefd) == -1) {
                perror("pipe failed");
                return 1;
            }
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            DEBUG_PRINTF("fork failed!\n");
            return -1;
        }
        
        if (pid == 0) {

            if (prev_read_fd != -1) {             // if not first cmd - read from saved read-end of pipe
                dup2(prev_read_fd, STDIN_FILENO); // otherwise - read from stdin
                close(prev_read_fd);
            }
            
            if (i < chld_proc_amount - 1) {     // if not last cmd - write to write-end of pipe
                dup2(pipefd[1], STDOUT_FILENO); // otherwise - write to stdout
                close(pipefd[0]);
                close(pipefd[1]);
            }
            
            execvp(dicts[i]->cmd, (char**)dicts[i]->args->data);
            perror("exec failed");
            exit(1);
        }
        
        if (prev_read_fd != -1) {
            close(prev_read_fd);
        }
        
        if (i < chld_proc_amount - 1) {
            close(pipefd[1]);
            prev_read_fd = pipefd[0]; 
        }
    }

    if (prev_read_fd != -1) {
        close(prev_read_fd);
    }

    for (size_t i = 1; i <= chld_proc_amount; i++) {
        int status = 0;
        wait(&status);
        DEBUG_PRINTF("Cmd %zd Ret code: %d\n", i, WEXITSTATUS(status));
    }

    DEBUG_PRINTF("All %zd processes ended\n", chld_proc_amount);
    
    for (size_t i = 0; i < chld_proc_amount; i++) {

        if (dicts[i]) {

            if (dicts[i]->args) {

                Dynamic_array_dtor(dicts[i]->args);
                free(dicts[i]->args);
            }
            
            if (dicts[i]->sep_args_ptrs) {
                
                char** sep_args_ptrs_array = (char**) dicts[i]->sep_args_ptrs->data;
                size_t sep_args_ptrs_amount = dicts[i]->sep_args_ptrs->size / sizeof(char*);
                for (size_t j = 0; j < sep_args_ptrs_amount; j++) 
                    if (sep_args_ptrs_array[j]) 
                        free(sep_args_ptrs_array[j]); 
                
                Dynamic_array_dtor(dicts[i]->sep_args_ptrs);
                free(dicts[i]->sep_args_ptrs);
            }

            free(dicts[i]);
        }
    }

    Dynamic_array_dtor(&dicts_d_array);
    free(cmd_buf_cpy);
}
