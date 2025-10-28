#ifndef SHARED_SEND
#define SHARED_SEND

#include <pthread.h>
#define SHM_SIZE (2<<23)
#define SHM_NAME_PREFIX "file_mem_"
#define SHM_NAME_SIZE 64

typedef struct shared_memory_t Shmem_;
typedef struct op_shared_memory_wrap_t Op_shmem;
typedef struct shared_memory_wrap_t Shmem;

struct op_shared_memory_wrap_t {
    int (*open)  (Shmem *self);
    int (*close) (Shmem *self);
    int (*destroy) (Shmem *self);
    int (*send_file) (Shmem *self, const char *src_file, const char *dst_file);
};

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    pthread_mutexattr_t m_attr;
    pthread_condattr_t  c_attr;
} mutex_and_cond;

struct shared_memory_t {
    void* buffer;
    mutex_and_cond prim;
    int max_size;
    int size;
};

struct shared_memory_wrap_t {
    int fd;
    char* name;
    Shmem_ *self;
    Op_shmem op;
};

Shmem *ctor_shm();
int    dtor_shm(Shmem *mem);

#endif