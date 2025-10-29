#ifndef FIFO_SEND
#define FIFO_SEND

#define FIFO_BUFFER_SIZE (1<<16)
#define FIFO_NAME_PREFIX "file_fifo_"
#define FIFO_NAME_SIZE 64

typedef struct fifo_t Fifo;
typedef struct op_fifo_t Op_fifo;

struct op_fifo_t {
    int (*open)    (Fifo *self);
    int (*close)   (Fifo *self);
};

struct fifo_t {
    char* name;
    int fd;
    int size;
    char* buffer;
    Op_fifo op;
};

Fifo *ctor_fifo();
int dtor_fifo(Fifo *fifo);
int send_file_fifo(Fifo *fifo, const char *src_file, const char *dst_file);

#endif
