#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "FullDuplexPipe.hpp"

size_t rcv(pPipe* self, int fd);
size_t snd(pPipe* self, int fd);

int create_pipe(pPipe* Pipe, size_t buf_size);
int destroy_pipe(pPipe* Pipe);

size_t rcv(pPipe* self, int fd) {
    return (self->len = (size_t)read(fd, self->data, self->size));
}

size_t snd(pPipe* self, int fd) {
    return (size_t)write(fd, self->data, self->len);
}

int create_pipe(pPipe* Pipe, size_t buf_size) {

    Pipe->size = buf_size;
    Pipe->data = (char*)calloc(Pipe->size, sizeof(char));
    if (!Pipe->data) {
        perror("calloc failed");
        return 1;
    }

    Pipe->actions.rcv = rcv;
    Pipe->actions.snd = snd;

    if (pipe(Pipe->fd_direct) == -1) {
        perror("pipe failed");
        return 1;
    }

    if (pipe(Pipe->fd_back) == -1) {
        perror("pipe failed");
        return 1;
    }

    return 0;
}

int destroy_pipe(pPipe* Pipe) {

    if (Pipe->data) {
        free(Pipe->data);
        Pipe->data = NULL;
    }

    return 0;
}
