#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "fifo_send.h"
#include "common.h"

int open_Fifo  (Fifo *Fifo_);
int close_Fifo (Fifo *Fifo_);
int recieve    (Fifo *Fifo_); 
int send       (Fifo *Fifo_);

int assign_commands(Op_fifo *self) {
    if (!self) return -1;

    self->open  = open_Fifo;
    self->close = close_Fifo;
    self->rcv   = recieve;
    self->snd   = send;

    return 0;
}
