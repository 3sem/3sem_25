#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define FIFO_NAME "test_fifo"

int main(int argc, char *argv[]) {
    if (argc != 4) return 1;
    const char *src = argv[1];
    const char *dst = argv[2];
    size_t buf_size = atol(argv[3]);

    mkfifo(FIFO_NAME, 0666);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();

    if (pid == 0) {
        int fd = open(FIFO_NAME, O_RDONLY);
        FILE *fout = fopen(dst, "wb");
        char *buf = malloc(buf_size);
        
        ssize_t n;
        while ((n = read(fd, buf, buf_size)) > 0) {
            fwrite(buf, 1, n, fout);
        }
        
        free(buf);
        fclose(fout);
        close(fd);
        exit(0);
    } else {
        int fd = open(FIFO_NAME, O_WRONLY);
        FILE *fin = fopen(src, "rb");
        char *buf = malloc(buf_size);
        
        size_t n;
        while ((n = fread(buf, 1, buf_size, fin)) > 0) {
            write(fd, buf, n);
        }
        
        free(buf);
        fclose(fin);
        close(fd);
        wait(NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("FIFO Time: %.4f s\n", seconds);

    unlink(FIFO_NAME);

    return 0;
}
