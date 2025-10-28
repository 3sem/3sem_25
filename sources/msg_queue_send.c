#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "msg_queue_send.h"
#include "common.h"

int open_Queue  (Queue *Queue_);
int close_Queue (Queue *Queue_);
int recieve    (Queue *Queue_); 
int send       (Queue *Queue_);

int assign_commands(Op_queue *self) {
    if (!self) return -1;

    self->open  = open_Queue;
    self->close = close_Queue;
    self->rcv   = recieve;
    self->snd   = send;

    return 0;
}
