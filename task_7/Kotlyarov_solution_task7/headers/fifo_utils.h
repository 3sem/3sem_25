#ifndef FIFO_UTILS
#define FIFO_UTILS

#define _GNU_SOURCE

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include "Debug_printf.h"

// definitely greater than max clients_id
#define FD_TYPE_CMD_FIFO (0xFFFFFFFF) 

int create_directory_recursive(const char* path);
int file_exists(const char* path);
int is_fifo(const char* path);
int create_fifo(const char* path);
int remove_fifo_and_empty_dirs(const char* path);
void cleanup_fifo_directory(const char* base_dir);

#endif
