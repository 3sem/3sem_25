#include "pipe.h"

ssize_t pipe_send_data(int dest_fd, void *buffer, size_t size) 
{
    return write(dest_fd, buffer, size);
}

ssize_t pipe_receive_data(int source_fd, void *buffer, size_t size) 
{
    return read(source_fd, buffer, size);
}

Pipe *dup_pipe_ctor ()
{
    Pipe *d_pipe = (Pipe *)calloc (1, sizeof(Pipe));
    
    if (pipe(d_pipe->fd_direct) == -1) {
        perror("Pipe direct failed");
        return NULL;
    }
    
    if (pipe(d_pipe->fd_back) == -1) {
        perror("Pipe back failed");
        close(d_pipe->fd_direct[0]);
        close(d_pipe->fd_direct[1]);
        return NULL;
    }

    d_pipe->actions.rcv = pipe_receive_data;
    d_pipe->actions.snd = pipe_send_data;

    return d_pipe;
}

void dup_pipe_dtor (Pipe *pipe)
{
    if (pipe->fd_direct[0] != -1) close(pipe->fd_direct[0]);
    if (pipe->fd_direct[1] != -1) close(pipe->fd_direct[1]);
    if (pipe->fd_back  [0] != -1) close(pipe->fd_back[0]);
    if (pipe->fd_back  [1] != -1) close(pipe->fd_back[1]);
        
    free(pipe);
}

void start_timing(struct timeval *start) 
{
    gettimeofday(start, NULL);
}

double stop_timing(struct timeval *start) 
{
    struct timeval end;
    gettimeofday(&end, NULL);
    return (end.tv_sec  - start->tv_sec) + 
           (end.tv_usec - start->tv_usec) / 1000000.0;
}

void parent_echo_send(Pipe *pipe, const char *input_filename)
{
    close(pipe->fd_direct[0]);
    close(pipe->fd_back[1]);
    
    char output_filename [MAX_NAME_SIZE] = {};
    snprintf(output_filename, sizeof(output_filename), "%s_copy", input_filename);
    
    int input_fd = open(input_filename, O_RDONLY);
    if (input_fd == -1) 
    {
        perror("Can't open input file");
        exit(1);
    }
    
    int output_fd = open(output_filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (output_fd == -1) 
    {
        perror("Can't open output file");
        close(input_fd);
        exit(1);
    }

    printf("---------------------------------------\n");
    printf("Transferring: %s -> %s\n", input_filename, output_filename);

    struct timeval start_time;
    start_timing(&start_time);

    char buf[BUF_SIZE] = {};
    ssize_t bytes_received = 0, bytes_written = 0;
    ssize_t total_sent = 0, total_rec = 0;

    while ((bytes_received = pipe->actions.rcv(input_fd, buf, BUF_SIZE-1)) > 0) 
    {
        bytes_written = pipe->actions.snd(pipe->fd_direct[1], buf, bytes_received);
        if (bytes_written != bytes_received)
        {
            perror("Write filed");
        }
        total_sent += bytes_written;

        while (total_rec < total_sent) 
        {
            bytes_received = pipe->actions.rcv(pipe->fd_back[0], buf, BUF_SIZE-1);
            bytes_written = pipe->actions.snd(output_fd, buf, bytes_received);
            if (bytes_written != bytes_received)
            {
                perror("Write filed");
            }
            total_rec += bytes_written;
        }
    }

    double copy_time = stop_timing(&start_time);

    printf("Transferred: %zd bytes\n", total_rec);
    printf("Echo test time: %.2f seconds\n", copy_time);
    printf("---------------------------------------\n");
    
    close(input_fd);
    close(output_fd);
}

void child_echo_back(Pipe *pipe)
{
    close(pipe->fd_direct[1]);
    close(pipe->fd_back[0]);

    char buffer[BUF_SIZE];
    ssize_t bytes_received = 0;

    while ((bytes_received = pipe->actions.rcv(pipe->fd_direct[0], buffer, BUF_SIZE-1)) > 0) 
    {
        pipe->actions.snd(pipe->fd_back[1], buffer, bytes_received);
    }
}