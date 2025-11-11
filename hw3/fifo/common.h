#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define FIFO_NAME "fifo"
#define BUF_SIZE 128*1024
#define OUT_FILE_NAME "output_file.txt"
#define IN_FILE_NAME "input_file.txt"