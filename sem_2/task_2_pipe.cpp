#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "task_2_pipe.h"

const int buffer_size = 4096;

size_t rcv (Pipe* self, pipeType parent) {

    if (parent == PARENT) {
        self->len = read(self->fd_back[0], self->data, buffer_size); //читаем из ребенка
    }
    else {
        self->len = read(self->fd_direct[0], self->data, buffer_size); //если ребенок, то читаем из родителя
    }
    return self->len;
}

size_t snd (Pipe* self, pipeType parent) {
    if (parent == PARENT) {
        return write(self->fd_direct[1], self->data, self->len);
    }
    else {
        return write(self->fd_back[1], self->data, self->len);

    }
}

size_t pipe_ctor (Pipe* self) {
    self->data = (char*)calloc(buffer_size, sizeof(char));
    if (self->data == NULL) {
        perror("Can't alloc buffer\n");
        return 1;
    }
    pipe(self->fd_direct);
    pipe(self->fd_back);
    self->len  = buffer_size;
    self->actions.rcv = &rcv;
    self->actions.snd = &snd;

    return 0;

}

void pipe_dtor (Pipe *self) {
    close(self->fd_direct[0]);
    close(self->fd_direct[1]);
    close(self->fd_back[0]);
    close(self->fd_back[1]);
    free(self->data);
}



