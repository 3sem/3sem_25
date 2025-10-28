#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "fifo_send.h"
#include "common.h"

int recieve_fifo (Fifo *Fifo_, int fd); 
int send_fifo    (Fifo *Fifo_, int fd);

int assign_commands_fifo(Op_fifo *self) {
    if (!self) return -1;

    self->rcv   = recieve_fifo;
    self->snd   = send_fifo;

    return 0;
}

Fifo *ctor_fifo() {
    Fifo *fifo;
    
    CALLOC_ERR(fifo, sizeof(*fifo), 0);
    if (assign_commands_fifo(&fifo->op) == -1) {
        free(fifo);
        perror("assign commands error\n");
        return 0;
    }
    CALLOC_CTOR(fifo->name, FIFO_NAME_SIZE, fifo, dtor_fifo, 0);
    sprintf(fifo->name, FIFO_NAME_PREFIX "pid_%d_addr_%p", getpid(), fifo);
    mknod(fifo->name, S_IFIFO | 0666, 0);

    return fifo;
}

int dtor_fifo(Fifo *fifo) {
    if (!fifo) return 0;


}
