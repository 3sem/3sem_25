#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAX_NAME_SIZE 100
#define BUF_SIZE (64 * 1024)



typedef struct op_table 
{
    ssize_t (*snd)(int dest_fd,   void *buffer, size_t size);
    ssize_t (*rcv)(int source_fd, void *buffer, size_t size);
} Ops;

typedef struct dup_pipe 
{
    int fd_direct[2];           // array of r/w descriptors for "pipe()" call (for parent-->child direction)
    int fd_back  [2];           // array of r/w descriptors for "pipe()" call (for child-->parent direction)
    Ops actions;
} Pipe;

typedef struct dup_pipe Pipe;
typedef struct op_table Ops;



ssize_t pipe_send_data(int dest_fd, void *buffer, size_t size);
ssize_t pipe_receive_data(int source_fd, void *buffer, size_t size);
void start_timing(struct timeval *start);
double stop_timing(struct timeval *start);
Pipe *dup_pipe_ctor ();
void dup_pipe_dtor (Pipe *pipe);
void parent_echo_send(Pipe *pipe, const char *input_filename);
void child_echo_back(Pipe *pipe);
