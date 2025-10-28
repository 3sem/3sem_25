#ifndef FIFO_SEND
#define FIFO_SEND

#define FIFO_NAME_PREFIX "file_fifo_"
#define FIFO_NAME_SIZE 64

typedef struct fifo_t Fifo;
typedef struct op_fifo_t Op_fifo;

struct op_fifo_t {
    int (*snd)   (Fifo *self, int fd);
    int (*rcv)   (Fifo *self, int fd);
};

struct fifo_t {
    char* name;
    int fd;
    Op_fifo op;
};

Fifo *ctor_fifo();
int dtor_fifo(Fifo *fifo);
int send_file_fifo(Fifo *fifo, const char *src_file, const char *dst_file);

#endif
