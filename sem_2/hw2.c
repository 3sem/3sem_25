/*
Разработать структуру, реализующую дуплексный канал межпроцессного взаимодействия на неименованных каналах (pipe).
Использовать при этом отдельные принципы построения программ на C в ООП-стиле.
Постараться максимизировать производительность эхо-теста с передачей от процесса-родителя к потомку и обратно больших файлов (> 4Гб) по частям. 
Метрика производительности - суммарное время, затраченное на передачу данных. Чем меньше, тем лучше.
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define CHUNK_SIZE (64 * 1024)

typedef struct {
    int parent_to_child[2];
    int child_to_parent[2];
    int read_fd;
    int write_fd;
} DuplexChannel;

DuplexChannel* DuplexChannel_create() {
    DuplexChannel* self = (DuplexChannel*)malloc(sizeof(DuplexChannel));
    if (!self) exit(1);
    
    if (pipe(self->parent_to_child) == -1 || pipe(self->child_to_parent) == -1) {
        free(self);
        exit(1);
    }
    return self;
}

void DuplexChannel_init_parent(DuplexChannel* self) {
    close(self->parent_to_child[0]); 
    close(self->child_to_parent[1]);
    
    self->write_fd = self->parent_to_child[1];
    self->read_fd  = self->child_to_parent[0];
}

void DuplexChannel_init_child(DuplexChannel* self) {
    close(self->parent_to_child[1]);
    close(self->child_to_parent[0]);
    
    self->read_fd  = self->parent_to_child[0];
    self->write_fd = self->child_to_parent[1];
}

ssize_t DuplexChannel_send(DuplexChannel* self, const void* data, size_t len) {
    size_t total_written = 0;
    const char* ptr = (const char*)data;
    
    while (total_written < len) {
        ssize_t written = write(self->write_fd, ptr + total_written, len - total_written);
        if (written == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_written += written;
    }
    return total_written;
}

ssize_t DuplexChannel_receive(DuplexChannel* self, void* data, size_t len) {
    size_t total_read = 0;
    char* ptr = (char*)data;
    
    while (total_read < len) {
        ssize_t n = read(self->read_fd, ptr + total_read, len - total_read);
        if (n == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;
        total_read += n;
    }
    return total_read;
}

void DuplexChannel_destroy(DuplexChannel* self) {
    if (self->read_fd != -1) close(self->read_fd);
    if (self->write_fd != -1) close(self->write_fd);
    free(self);
}

void child_process(DuplexChannel* channel) {
    DuplexChannel_init_child(channel);
    
    char* buffer = (char*)malloc(CHUNK_SIZE);
    if (!buffer) exit(1);

    ssize_t bytes_read;
    while ((bytes_read = DuplexChannel_receive(channel, buffer, CHUNK_SIZE)) > 0) {
        DuplexChannel_send(channel, buffer, bytes_read);
    }

    free(buffer);
    DuplexChannel_destroy(channel);
    exit(0);
}

void parent_process(DuplexChannel* channel, const char* input_file, const char* output_file) {
    DuplexChannel_init_parent(channel);

    FILE* fin = fopen(input_file, "rb");
    FILE* fout = fopen(output_file, "wb");
    if (!fin || !fout) exit(1);

    char* buffer = (char*)malloc(CHUNK_SIZE);
    if (!buffer) exit(1);

    size_t bytes_read;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fin)) > 0) {
        DuplexChannel_send(channel, buffer, bytes_read);
        DuplexChannel_receive(channel, buffer, bytes_read);
        fwrite(buffer, 1, bytes_read, fout);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    fseek(fin, 0L, SEEK_END);
    long sz = ftell(fin);
    printf("Time: %.3f s, Speed: %.2f GB/s\n", time_taken, (double)sz / (1024*1024*1024) / time_taken);

    free(buffer);
    fclose(fin);
    fclose(fout);
    DuplexChannel_destroy(channel);
    wait(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 3) return 1;

    DuplexChannel* channel = DuplexChannel_create();
    pid_t pid = fork();

    if (pid < 0) return 1;
    if (pid == 0) child_process(channel);
    else parent_process(channel, argv[1], argv[2]);

    return 0;
}
