#ifndef DUPLEX_PIPE
#define DUPLEX_PIPE

#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "Debug_printf.h"

typedef struct d_pipe Duplex_pipe;

typedef struct pipe_methods Pipe_methods;

typedef struct pipe_methods  {
    
    int64_t (*recieve)(Duplex_pipe *self, int opt);
    int64_t (*send)(Duplex_pipe *self, int opt);
    bool    (*close_pipefd)(Duplex_pipe *self, int pipefd_opt);
    void    (*close_unused_pipefd)(Duplex_pipe* pipe, int pipefd_opt);
    bool    (*dtor)(Duplex_pipe *self);
} Pipe_methods;

typedef struct d_pipe {
        char* tmp_buf; 
        size_t tmp_buf_capacity; // data length in intermediate buffer
        size_t tmp_buf_size; // data length in intermediate buffer
        int pipefd_direct[2]; // array of r/w descriptors for "pipe()" call (for parent-->child direction)
        int pipefd_reverse[2]; // array of r/w descriptors for "pipe()" call (for child-->parent direction)
        Pipe_methods methods;
} Duplex_pipe;

static const int Tmp_buf_def_size = 1024;
static const int64_t Recieve_error_val = -0xDEAD;
static const int64_t Send_error_val  = -0xBAD;
#define PRNT_PIPE 0
#define CHLD_PIPE 1
#define RD_PIPE_END 0
#define WR_PIPE_END 1
// these options are more common for pipe descriptors
#define RD_DIR 0
#define WR_DIR 1
#define RD_REV 2
#define WR_REV 3
// these options are more intuitive for methods, so different names are for the same values
#define RCV_DIR 4
#define SND_DIR 5
#define RCV_REV 6
#define SND_REV 7

Duplex_pipe* duplex_pipe_ctor(size_t tmp_buf_size);
void duplex_pipe_close_unused_pipefd(Duplex_pipe* pipe, int pipefd_opt);
bool duplex_pipe_close_pipefd(Duplex_pipe* pipe, int pipefd_opt);
int64_t duplex_pipe_recieve(Duplex_pipe* pipe, int recieve_opt);
int64_t duplex_pipe_send(Duplex_pipe* pipe, int send_opt);
bool duplex_pipe_dtor(Duplex_pipe* pipe);
#endif
