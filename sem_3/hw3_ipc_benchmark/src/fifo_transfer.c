#include "common.h"
#include "fifo_transfer.h"

void fifo_copy_file (const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Fopen failed");
        return;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unlink(FIFO_NAME);
    mkfifo(FIFO_NAME, 0666);

    pid_t pid = fork();
    
    if (pid == 0) {
        char output_filename[MAX_FILENAME];
        snprintf(output_filename, MAX_FILENAME, "copy_%s", filename);
        
        FILE *output_file = fopen(output_filename, "wb");
        int fd = open(FIFO_NAME, O_RDONLY);

        char buffer[BUF_SIZE];
        size_t total_received = 0;
        ssize_t bytes_read;
        
        while ((bytes_read = read(fd, buffer, BUF_SIZE)) > 0) {
            fwrite(buffer, 1, bytes_read, output_file);
            total_received += bytes_read;
        }
        
        close(fd);
        fclose(output_file);
        unlink(FIFO_NAME);
        exit(0);
    } 
    else 
    {
        printf("---------------------------------------\n");
        printf("Starting transfer...\n");
        printf("File size: %ld bytes\n", file_size);
        
        int fd = open(FIFO_NAME, O_WRONLY);
        size_t total_sent = 0;
        size_t chunks_total = (file_size + BUF_SIZE - 1) / BUF_SIZE;

        struct timeval start_time;
        start_timing(&start_time);

        char buffer[BUF_SIZE];
        
        for (size_t chunk_num = 0; chunk_num < chunks_total; chunk_num++) {
            size_t bytes_read = fread(buffer, 1, BUF_SIZE, file);
            write(fd, buffer, bytes_read);
            total_sent += bytes_read;
        }

        close(fd);
        wait(NULL);

        double copy_time = stop_timing(&start_time);
        printf("Transfer completed (%ld bytes) in %.2f seconds\n", total_sent, copy_time);
        printf("---------------------------------------\n");
        
        fclose(file);
    }
}