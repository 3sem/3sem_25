#define _POSIX_C_SOURCE 199309L
#include "common.h"
#include <time.h>

int main() 
{
    char buffer[BUF_SIZE];
    
    int fd = open(IN_FILE_NAME, O_RDONLY);
    struct stat st;
    fstat(fd, &st);
    size_t file_size = st.st_size;
    
    access(FIFO_NAME, F_OK);
    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    
    size_t current_chunk = (file_size < BUF_SIZE) ? file_size : BUF_SIZE;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    long long total_bytes = 0;
    size_t bytes_read = 0;
    while ((bytes_read = read(fd, buffer, current_chunk)) > 0) 
    {
        write(fifo_fd, buffer, bytes_read);

        size_t remaining = file_size - total_bytes;
        current_chunk = (remaining < BUF_SIZE) ? remaining : BUF_SIZE;
    }
    close(fd);
    close(fifo_fd);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("time: %.3f seconds\n", time_taken);
    
    return 0;
}