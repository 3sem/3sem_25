#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

const size_t MAX_BUF_SIZE = 64 * 1024;
const char* FILE_NAME = "file.txt";
const char* OUTPUT_FILE_NAME = "output.txt";

typedef struct DPOps
{
    size_t (*dp_read)(int fd, char* buf, size_t size);
    size_t (*dp_write)(int fd, const char* buf, size_t size);
} DPOps;

typedef struct DuplexPipe
{
    char* buf;
    size_t buf_size;
    int ctop_pipe_fd[2];
    int ptoc_pipe_fd[2];
    DPOps ops;
} DP;

DP* dp_ctor();
void dp_dtor(DP* dpipe);
size_t dp_read(int fd, char* buf, size_t size);
size_t dp_write(int fd, const char* buf, size_t size);
void ctop_echo(DP* dpipe);
void ftop_ptof(DP* dpipe, int write_fd, int read_fd);

int main()
{
    struct timespec start, end;

    DP* dpipe = dp_ctor();
    pid_t pid = fork();

    if (pid == 0) 
    {
        close(dpipe->ctop_pipe_fd[0]);
        close(dpipe->ptoc_pipe_fd[1]);
        
        ctop_echo(dpipe);
        
        close(dpipe->ptoc_pipe_fd[0]);
        close(dpipe->ctop_pipe_fd[1]);
    }
    else 
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        close(dpipe->ctop_pipe_fd[1]);
        close(dpipe->ptoc_pipe_fd[0]);

        ftop_ptof(dpipe, dpipe->ptoc_pipe_fd[1], dpipe->ctop_pipe_fd[0]);

        close(dpipe->ptoc_pipe_fd[1]);
        close(dpipe->ctop_pipe_fd[0]);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
        printf("time: %.3f seconds\n", time_taken);
    }

    dp_dtor(dpipe);

    return 0;
}

DP* dp_ctor()
{
    DP* dpipe = (DP*)calloc(1, sizeof(DP));

    pipe(dpipe->ctop_pipe_fd);
    pipe(dpipe->ptoc_pipe_fd); 

    dpipe->buf = (char*)calloc(MAX_BUF_SIZE, sizeof(char));
    dpipe->buf_size = MAX_BUF_SIZE;
    
    dpipe->ops.dp_read = dp_read;
    dpipe->ops.dp_write = dp_write;

    return dpipe;
}

void dp_dtor(DP* dpipe)
{
    if (dpipe) 
    {
        free(dpipe->buf);
        free(dpipe);
    }
}

size_t dp_read(int fd, char* buf, size_t size)
{
    return read(fd, buf, size);
}

size_t dp_write(int fd, const char* buf, size_t size)
{
    return write(fd, buf, size);
}

void ftop_ptof(DP* dpipe, int write_fd, int read_fd)
{
    int in_fd = open(FILE_NAME, O_RDONLY);
    int out_fd = open(OUTPUT_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    size_t bytes_read = 0;
    char* temp_buf = (char*)calloc(MAX_BUF_SIZE, sizeof(char));
    
    while ((bytes_read = read(in_fd, dpipe->buf, dpipe->buf_size)) > 0) 
    {
        write(write_fd, dpipe->buf, bytes_read);
        
        size_t total_received = 0;
        while (total_received < bytes_read) 
        {
            size_t received = read(read_fd, temp_buf, bytes_read - total_received);
            write(out_fd, temp_buf, received);
            total_received += received;
        }
    }
    
    free(temp_buf);

    close(in_fd);
    close(out_fd);
}

void ctop_echo(DP* dpipe) 
{
    size_t bytes_read = 0;
    
    while ((bytes_read = dpipe->ops.dp_read(dpipe->ptoc_pipe_fd[0], dpipe->buf, dpipe->buf_size)) > 0) 
    {
        size_t bytes_written = dpipe->ops.dp_write(dpipe->ctop_pipe_fd[1], dpipe->buf, bytes_read);
    }
}