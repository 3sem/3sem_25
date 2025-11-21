#include "common.h"

int main() 
{
    char buffer[BUF_SIZE];
    
    mkfifo(FIFO_NAME, 0666);
    int fifo_fd = open(FIFO_NAME, O_RDONLY);
    int output_fd = open(OUT_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    size_t bytes_read = 0;
    while ((bytes_read = read(fifo_fd, buffer, BUF_SIZE)) > 0) 
        write(output_fd, buffer, bytes_read);
    
    close(output_fd);
    close(fifo_fd);
    
    unlink(FIFO_NAME);
    
    return 0;
}