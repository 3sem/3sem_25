#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "parse_cmd.h"

const size_t Max_cmd_buf_size = 2097152;

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
    cmd_args_dict* dict = parse_cmd(cmd_buf, cmd_buf_ptr);
    DEBUG_PRINTF("cmd: %s\n", dict->cmd);
    DEBUG_PRINTF("args: ");
    char* arg_ptr = *((char**)dict->args->data);
    DEBUG_PRINTF("%s\n", arg_ptr);   
    for(int i = 1; arg_ptr; i++) {

        arg_ptr = *((char**)dict->args->data + i);    
        DEBUG_PRINTF("      %s\n", arg_ptr);   
    }

    DEBUG_PRINTF("args amout: %zd\n", dict->args_amount);   
    //execvp(dict->cmd, (char**)dict->args->data); 
    char** tmp_ptr = (char**)dict->sep_args_ptrs->data;
    for (size_t i = 0; i < dict->sep_args_ptrs->size / sizeof(char*); i++) {

        DEBUG_PRINTF("*tmp_ptr: %s\n", *(tmp_ptr + i));
        free(*(tmp_ptr + i));
    }
    free(dict->args->data);
    free(dict->args);
    Dynamic_array_dtor(dict->sep_args_ptrs);
    free(dict);
    dict = parse_cmd(NULL, cmd_buf_ptr);
    /*DEBUG_PRINTF("cmd: %s\n", dict->cmd);
    DEBUG_PRINTF("args: ");
    arg_ptr = *((char**)dict->args->data);
    DEBUG_PRINTF("%s\n", arg_ptr);   
    for(int i = 1; arg_ptr; i++) {

        arg_ptr = *((char**)dict->args->data + i);    
        DEBUG_PRINTF("      %s\n", arg_ptr);   
    }

    DEBUG_PRINTF("args amout: %zd\n", dict->args_amount);
    DEBUG_PRINTF("*tmp_ptr = %p\n", *tmp_ptr);
    DEBUG_PRINTF("dict->args->data = %p\n", dict->args->data);
    DEBUG_PRINTF("dict->args = %p\n", dict->args);
    DEBUG_PRINTF("dict = %p\n", dict);
    DEBUG_PRINTF("cmd_buf = %p\n", cmd_buf);
    tmp_ptr = (char**)dict->sep_args_ptrs->data;
    for (size_t i = 0; i < dict->sep_args_ptrs->size / sizeof(char*); i++) {

        DEBUG_PRINTF("*tmp_ptr: %s\n", *(tmp_ptr + i));
        free(*(tmp_ptr + i));
    }
    free(dict->args->data);
    free(dict->args);
    Dynamic_array_dtor(dict->sep_args_ptrs);
    free(dict);*/
    free(cmd_buf_cpy);

}
