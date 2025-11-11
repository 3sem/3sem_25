#ifndef MQ_TRANSFER_H_
#define MQ_TRANSFER_H_

#include "common.h"

#define MSG_TYPE_DATA 1
#define MSG_TYPE_END 2

typedef struct {
    long msg_type;
    size_t bytes_used;
    char data[BUF_SIZE];
} message_t;

void mq_copy_file (const char *filename);

#endif