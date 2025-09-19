#pragma once

#include <stdio.h>

#define BUF_SIZE 	4096
#define READ_PIPE 	0
#define WRITE_PIPE 	1

#define FILE_NAME	"file.txt"
#define FILE_NEW	"new_"

#define PIPE_RCV(fd) Pipe->actions.rcv(Pipe, fd)
#define PIPE_SND(fd) Pipe->actions.snd(Pipe, fd)

typedef struct pPipe Pipe;
typedef struct op_table Ops;

typedef struct op_table {
    size_t (*rcv)(Pipe *self, int fd);
    size_t (*snd)(Pipe *self, int fd);
} Ops;

typedef struct pPipe {
	char* data;
	int fd_direct[2];
	int fd_back[2];
	size_t len;
	size_t size;
	Ops actions;
} Pipe;
