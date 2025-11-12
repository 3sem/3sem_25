#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#define FIFO_NAME "fifo_channel"
#define BUFFER_SIZE 1048576

static double timespec_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл_для_записи>\n", argv[0]);
        return 1;
    }

    mkfifo(FIFO_NAME, 0666);

    int input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        perror("Ошибка открытия файла для чтения");
        return 1;
    }

    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    if (fifo_fd < 0) {
        perror("Ошибка открытия FIFO для записи");
        close(input_fd);
        return 1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
        write(fifo_fd, buffer, bytes_read);
    
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = timespec_diff(start, end);

    printf("Время передачи (отправитель): %.6f секунд\n", elapsed);

    close(input_fd);
    close(fifo_fd);

    return 0;
}

