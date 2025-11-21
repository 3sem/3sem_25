#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#define MAX_MSG_SIZE 7*1024
#define BUF_SIZE 128*1024
#define KEY 8
#define IN_FILE_NAME "input_file.txt"
#define OUT_FILE_NAME "output_file.txt"

#define MSG_DATA 1
#define MSG_END 2
#define MSG_ACK  3 

struct message 
{
    long mtype;        
    size_t data_size; 
    char data[MAX_MSG_SIZE];
};