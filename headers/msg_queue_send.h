#ifndef QUEUE_SEND
#define QUEUE_SEND

#define QUEUE_SIZE 65536

typedef struct op_message_queue_t Queue;
typedef struct op_message_queue_t Op_queue;

struct op_message_queue_t {
    int (*open)  (Queue *self);
    int (*close) (Queue *self);
    int (*snd)   (Queue *self);
    int (*rcv)   (Queue *self);
};

struct message_queue_t {
    Op_queue op;
};

Queue *ctor_queue();
int dtor_queue(Queue *queue);
int send_file_queue(Queue *queue, const char *src_file, const char *dst_file);

#endif