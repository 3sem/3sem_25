#pragma once

#include <stdio.h>

#define BUF_SIZE 	4096
#define READ_PIPE 	0
#define WRITE_PIPE 	1

#define FILE_NAME	"file.txt"
#define FILE_NEW	"new_"
#define FILE_CREATE "time dd if=/dev/urandom of=" FILE_NAME " bs=1048576 count=4096"

#define PIPE_RCV(fd) Pipe->actions.rcv(Pipe, fd)
#define PIPE_SND(fd) Pipe->actions.snd(Pipe, fd)

struct pPipe;
typedef struct op_table Ops;

typedef struct op_table {
    size_t (*rcv)(pPipe *self, int fd);
    size_t (*snd)(pPipe *self, int fd);
} Ops;

struct pPipe {
	char* data;
	int fd_direct[2];
	int fd_back[2];
	size_t len;
	size_t size;
	Ops actions;
};
