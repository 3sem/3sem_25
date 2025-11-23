#ifndef CHECK_R_W_FLAGS_H
#define CHECK_R_W_FLAGS_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>


#define MAX_READ_FILES 16
#define MAX_WRITE_FILES 8
#define MAX_FILENAME_LENGTH 256

typedef struct {
    char** read_files;
    int read_files_count;
    char** write_files;
    int write_files_count;
    int check_status;
    bool has_errors;
    char error_message[256];
} file_types;

enum Check_flags_option {
    CHECK_NONE = 0x0,
    CHECK_R = 0x1,
    CHECK_W = 0x2,
    CHECK_R_W = CHECK_R | CHECK_W,
    OPTIONAL_CHECK = 0x4
};

bool check_r_w_flags(int check_option, char** argv, int argc, file_types* files);
void init_file_types(file_types* files);
void cleanup_file_types(file_types* files);
void print_file_types(const file_types* files);

bool validate_filename(const char* filename);
bool add_read_file(file_types* files, const char* filename);
bool add_write_file(file_types* files, const char* filename);
const char* get_check_status_string(int check_status);
void print_usage(const char* program_name);

#endif
