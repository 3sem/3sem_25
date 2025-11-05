#ifndef PARSE_CMD
#define PARSE_CMD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Debug_printf.h"
#include "Dynamic_array_funcs.h"

struct cmd_args_dict {

    char* cmd;
    Dynamic_array* args;
    size_t args_amount;
    Dynamic_array* sep_args_ptrs;
};

typedef struct cmd_args_dict cmd_args_dict; 

static const int D_array_init_size = 5;

#define D_ARRAY_PUSH_PTR(d_array, ptr) Dynamic_array_push_back(d_array, &ptr, sizeof(void*));

cmd_args_dict* parse_cmd(char* cmd_string, char** cmd_string_ptr);
char* separate_args(char* arg, Dynamic_array* d_array, size_t* args_amount_ptr); 

#endif
