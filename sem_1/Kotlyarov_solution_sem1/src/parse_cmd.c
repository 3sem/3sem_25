#include "parse_cmd.h"

cmd_args_dict* parse_cmd(char* cmd_string, char** cmd_string_ptr) {

    
    if (!cmd_string_ptr || !(*cmd_string_ptr)) {

        DEBUG_PRINTF("ERROR: null ptr\n");
        return NULL; 
    }

    cmd_args_dict* dict = (cmd_args_dict*) calloc(1, sizeof(cmd_args_dict));

    if (!dict) {

        DEBUG_PRINTF("ERROR: memory was not allocated\n");
        return NULL; 
    }
    
    char* curr_cmd = strtok_r(cmd_string, "|", cmd_string_ptr);
    if (!curr_cmd) {

        free(dict);
        return NULL;
    }
    //DEBUG_PRINTF("curr_cmd full: %s\n", curr_cmd);
    char** curr_cmd_ptr = &curr_cmd;
    char* exec_file = strtok_r(curr_cmd, " ", curr_cmd_ptr); 
    if (!exec_file) {

        free(dict);
        return NULL;
    }

    dict->cmd = strdup(exec_file);
    if (!dict->cmd) {

        free(dict);
        return NULL;
    }
    //DEBUG_PRINTF("curr_cmd: %s\n", dict->cmd);
    Dynamic_array* d_array = (Dynamic_array*) calloc(1, sizeof(Dynamic_array));
    Dynamic_array* d_array_sep_args_ptrs = (Dynamic_array*) calloc(1, sizeof(Dynamic_array));
    if (!d_array_sep_args_ptrs) {
        
        if (d_array) 
            free(d_array);
        
        free(dict);
        DEBUG_PRINTF("ERROR: memory was not allocated\n");
        return NULL;
    }
    
    if (!Dynamic_array_ctor(d_array, D_array_init_size, 0)) {
        
        free(d_array);            
        free(d_array_sep_args_ptrs);
        free(dict);
        return NULL;
    }

    if(!Dynamic_array_ctor(d_array_sep_args_ptrs, D_array_init_size, 0)){
        
        Dynamic_array_dtor(d_array);
        free(d_array);            
        free(d_array_sep_args_ptrs);
        free(dict);
        return NULL;
    }
    
    D_ARRAY_PUSH_PTR(d_array, dict->cmd);
    dict->args_amount = 1;
    char* token = NULL;
    while ((token = strtok_r(NULL, " ", curr_cmd_ptr))) {

        if (*token == '-' && *(token + 1) != '-' && !isspace(*(token + 1))) {

            char* sep_args_ptr = separate_args(token, d_array, &dict->args_amount);
            if (!sep_args_ptr) {

                Dynamic_array_dtor(d_array);
                Dynamic_array_dtor(d_array_sep_args_ptrs);
                free(d_array);
                free(d_array_sep_args_ptrs);
                free(dict);
                return NULL;
            }

            D_ARRAY_PUSH_PTR(d_array_sep_args_ptrs, sep_args_ptr); 
            
        }

        else{

            D_ARRAY_PUSH_PTR(d_array, token);
            dict->args_amount++;
        }

    }

    D_ARRAY_PUSH_PTR(d_array, token);
    dict->args = d_array;
    dict->sep_args_ptrs = d_array_sep_args_ptrs;

    return dict;
}

char* separate_args(char* arg, Dynamic_array* d_array, size_t* args_amount_ptr) {

    Dynamic_array d_array_sep_args = {};
    Dynamic_array* d_array_sep_args_ptr = &d_array_sep_args;  
    if (!Dynamic_array_ctor(&d_array_sep_args, 3*(strlen(arg)), 0)) // space for each '-' '{char}' '\0'
        return NULL;

    for (int i = 1; isalnum(arg[i]); i++) {
       
        char* arg_ptr = d_array_sep_args.data + d_array_sep_args.size;
        D_ARRAY_PUSH_PTR(d_array, arg_ptr);
        INSERT_BYTE_BY_PTR(d_array_sep_args_ptr, arg + 0);
        INSERT_BYTE_BY_PTR(d_array_sep_args_ptr, arg + i);
        INSERT_BYTE(d_array_sep_args_ptr, '\0');
        (*args_amount_ptr)++;
    }
    
    return d_array_sep_args.data;
}

