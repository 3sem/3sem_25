#ifndef QUEUE_SEND
#define QUEUE_SEND

#define MSG_SIZE 8184

typedef struct message_mqueue_t Mqueue;

typedef struct msgbuf {
    long mtype;
    int  size;
    char data[MSG_SIZE];
} message_buf;

struct message_mqueue_t {
    message_buf mbuf;
    int msqid;
    int msgkey;
};

Mqueue *ctor_mqueue();
int dtor_mqueue(Mqueue *mqueue);
int send_file_mqueue(Mqueue *mqueue, const char *src_file, const char *dst_file);

#endif
