#include <stddef.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "parse_cmd.h"

int main() {

    char* input_buf = NULL, * work_buf = NULL;
    size_t input_buf_size = 0;
    ssize_t read = getline(&input_buf, &input_buf_size, stdin);
    while (read > 0) {
        
        work_buf = strndup(input_buf, read);
        if (!work_buf) {
            free(input_buf);
            return 1;
        }
        
        char* work_buf_cpy = work_buf;

        while (read > 0 && (work_buf[read - 1] == '\n' || work_buf[read - 1] == '\r')) {
            work_buf[--read] = '\0';
        }

        int prev_read_fd = -1;
        Dynamic_array dicts_d_array = {};
        Dynamic_array* dicts_d_array_ptr = &dicts_d_array;
        if (!Dynamic_array_ctor(dicts_d_array_ptr, D_array_init_size, 0))
            return 1;
        
        char** work_buf_ptr = &work_buf;
        cmd_args_dict* dict = parse_cmd(work_buf, work_buf_ptr);
        D_ARRAY_PUSH_PTR(dicts_d_array_ptr, dict);
        while ((dict = parse_cmd(NULL, work_buf_ptr))) {
            D_ARRAY_PUSH_PTR(dicts_d_array_ptr, dict);
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
                
                if (dicts[i]->cmd) 
                    free(dicts[i]->cmd);

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
        
        free(work_buf_cpy);
        work_buf = NULL;
        Dynamic_array_dtor(&dicts_d_array);
        
        read = getline(&input_buf, &input_buf_size, stdin);
    }

    free(input_buf);
    return 0;
}
