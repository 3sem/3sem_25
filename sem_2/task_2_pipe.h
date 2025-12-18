#include <stdio.h>
#include <stdlib.h>


typedef enum pType {
    CHILD = 0,
    PARENT = 1,
} pipeType;

typedef struct pPipe Pipe;

typedef struct op_table  {
    size_t (*rcv)(Pipe *self, pipeType);
    size_t (*snd)(Pipe *self, pipeType);
} Ops;

typedef struct pPipe {
        char* data; // intermediate buffer
        int fd_direct[2]; // array of r/w descriptors for "pipe()" call (for parent-->child direction)
        int fd_back[2]; // array of r/w descriptors for "pipe()" call (for child-->parent direction)
        size_t len; // data length in intermediate buffer
        Ops actions;
} Pipe;

double parent_process(Pipe* pipe, FILE* input_file, FILE* output_file);
void child_process(Pipe* pipe);

size_t pipe_ctor (Pipe* self);
void pipe_dtor (Pipe *self);


