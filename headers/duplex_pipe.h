#ifndef DUPLEX_PIPE
#define DUPLEX_PIPE

#include <stdio.h>
#include <stdlib.h>

#define BUF_SZ 65536

typedef struct pPipe Pipe;
typedef struct op_pipe_table Op_pipe;

typedef enum {
    DIRECT =  1,
    BACK   =  2,
    UNUSED =  0,
    BROKEN = -1,
} Pipe_type;

typedef struct op_pipe_table  {
    int (*open)  (Pipe *self);
    int (*close) (Pipe *self);
    int (*swap)  (Pipe *self);
    int (*rcv)   (Pipe *self);
    int (*snd)   (Pipe *self);
} Op_pipe;

typedef struct pPipe {
        char* data;
        int fd_write; // array of r/w descriptors for "pipe()" call (for parent-->child direction)
        int fd_read;  // array of r/w descriptors for "pipe()" call (for child-->parent direction)
        size_t len;
        Pipe_type status;
        Op_pipe actions;
} Pipe;

Pipe *ctor_Pipe();
int dtor_Pipe(Pipe * pipe_);
int recieve_file(Pipe *pipe_, const char *file_name);
int send_file   (Pipe *pipe_, const char *file_name);

#endif