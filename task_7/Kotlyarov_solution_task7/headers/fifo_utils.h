#ifndef FIFO_UTILS
#define FIFO_UTILS

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

int create_fifo_with_dirs(const char* path);
int remove_directory(const char* path);
int remove_fifo_and_empty_dirs(const char* path);
void cleanup_fifo_directory(const char* base_dir);
int file_exists(const char* path);
int is_fifo(const char* path);
int setup_client_fifos(const char* tx_path, const char* rx_path);
void cleanup_client_fifos(const char* tx_path, const char* rx_path);

#endif
