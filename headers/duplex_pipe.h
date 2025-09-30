#ifndef DUPLEX_PIPE
#define DUPLEX_PIPE

#include <stdio.h>
#include <stdlib.h>

#define BUF_SZ 4096

typedef struct pPipe Pipe;
typedef struct op_pipe_table Op_pipe;

typedef struct op_pipe_table  {
    size_t (*rcv)(Pipe *self); 
    size_t (*snd)(Pipe *self); 
} Op_pipe;

typedef struct pPipe {
        char* data;
        int fd_direct[2]; // array of r/w descriptors for "pipe()" call (for parent-->child direction)
        int fd_back[2]; // array of r/w descriptors for "pipe()" call (for child-->parent direction)
        size_t len;
        Op_pipe actions;
} Pipe;

#endif