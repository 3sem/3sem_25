#include <memory.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "shared_mem_send.h"
#include "common.h"

int call_shm_open   (Shmem *mem);
int call_shm_close (Shmem *mem);
int call_shm_destroy (Shmem *self);
int send_file_shm (Shmem *mem, const char *src_file, const char *dst_file);

int assign_commands_shm(Op_shmem *self) {
    if (!self) return -1;

    self->open  = call_shm_open;
    self->close = call_shm_close;
    self->destroy = call_shm_destroy;
    self->send_file  = send_file_shm;

    return 0;
}

int init_opened_shm(Shmem_ *mem_) {
    mem_->buffer = (void*)(mem_+1);
    mem_->size = 0;
    mem_->max_size = SHM_SIZE - sizeof(*mem_);

    pthread_condattr_init(&mem_->prim.c_attr);
    pthread_condattr_setpshared(&mem_->prim.c_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&mem_->prim.cond, &mem_->prim.c_attr);
    pthread_mutexattr_init(&mem_->prim.m_attr);
    pthread_mutexattr_setpshared(&mem_->prim.m_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mem_->prim.mutex, &mem_->prim.m_attr);

    return 0;
}

Shmem *ctor_shm() {
    Shmem *mem;

    CALLOC_SELF(mem);
    if (assign_commands_shm(&mem->op) == -1) {
        free(mem);
        perror("assign commands error\n");
        return 0;
    }
    CALLOC_DTOR(mem->name, SHM_NAME_SIZE, mem, dtor_shm, 0);
    sprintf(mem->name, SHM_NAME_PREFIX "pid_%d_addr_%p", getpid(), mem);

    return mem;
}
int dtor_shm(Shmem *mem) {
    if (!mem) return 0;

    if (mem->op.close(mem) == -1) perror("close shm error\n");

    if (mem->name) {
        memset(mem->name, 0, SHM_NAME_SIZE);
        free(mem->name);
    }

    SET_ZERO(mem);
    free(mem);

    return 0;
}

int call_shm_open(Shmem *mem) {
    if (!mem) {
        perror("no mem object\n");
        return -1;
    }
    if (mem->fd != 0) {
        perror("mem fd already exists\n");
        return -1;
    }

    if ((mem->fd = shm_open(mem->name, O_CREAT | O_RDWR, 0666)) < 0) {
        perror("open shared memory error\n");
        return -1;
    }
    if (ftruncate(mem->fd, SHM_SIZE) == -1) {
        perror("ftruncate error\n");
        return -1;
    }

    if ((mem->self = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, mem->fd, 0)) == MAP_FAILED) {
        perror("mmap error\n");
        return -1;
    }
    if (init_opened_shm(mem->self)) {
        perror("init opened shared memory error\n");
        return -1;
    }

    return 0;
}
int call_shm_close(Shmem *mem) {
    if (!mem) {
        perror("no mem object (close)\n");
        return -1;
    }
    if (mem->fd <= 0) return 0;

    if (munmap(mem->self, SHM_SIZE) == -1) {
        perror("munmap error\n");
        return -1;
    }

    mem->self = NULL;
    close(mem->fd);
    mem->fd = 0;

    return 0;
}
int call_shm_destroy(Shmem *mem) {
    if (!mem) {
        perror("no mem object\n");
        return -1;
    }

    if (mem->fd <= 0) {
        perror("memory fd not exist\n");
        return -1;
    }

    pthread_mutex_destroy(&mem->self->prim.mutex);
    pthread_mutexattr_destroy(&mem->self->prim.m_attr);
    pthread_mutex_destroy(&mem->self->prim.mutex);
    pthread_mutexattr_destroy(&mem->self->prim.m_attr);

    SET_ZERO(mem->self);

    mem->op.close(mem);

    shm_unlink(mem->name);

    dtor_shm(mem);

    return 0;
}

int recieve_all_shm(Shmem_ *mem_, int fd) {
    pthread_mutex_lock(&mem_->prim.mutex);
    while (mem_->size != -1) {
        while (mem_->size != 0) {
            pthread_cond_wait(&mem_->prim.cond, &mem_->prim.mutex);
        }


        mem_->size = read(fd, mem_->buffer, mem_->max_size);
        if (mem_->size == 0) mem_->size = -1;

        pthread_cond_signal(&mem_->prim.cond);
    }
    pthread_mutex_unlock(&mem_->prim.mutex);
    // printf("recieve success\n");
    return 0;
}
int send_all_shm(Shmem_ *mem_, int fd) {
    pthread_mutex_lock(&mem_->prim.mutex);
    if (mem_->size == 0) pthread_cond_wait(&mem_->prim.cond, &mem_->prim.mutex);
    while (mem_->size != -1) {

        int shift = 0;
        while (mem_->size != shift) {
            shift += write(fd, mem_->buffer + shift, mem_->size - shift);
        }

        mem_->size = 0;
        pthread_cond_signal(&mem_->prim.cond);

        while (mem_->size == 0) {
            pthread_cond_wait(&mem_->prim.cond, &mem_->prim.mutex);
        }
    }
    pthread_mutex_unlock(&mem_->prim.mutex);
    // printf("send success\n");
    return 0;
}

int send_file_shm (Shmem *mem, const char *src_file, const char *dst_file) {
    if (!mem) {
        perror("no mem object\n");
        return -1;
    }
    if (!src_file || !dst_file) {
        perror("broken names\n");
        return -1;
    }

    int fd_dst, fd_src;

    if ((fd_src = open(src_file, O_RDONLY | O_CREAT, 0666)) == -1) {
        perror("open source file failed\n");
        return -1;
    }
    if ((fd_dst = open(dst_file, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
        perror("open destination file failed\n");
        close(fd_src);
        return -1;
    }

    SET_NONBLOCK(fd_src);
    SET_NONBLOCK(fd_dst);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed (shm)\n");
        close(fd_src);
        close(fd_dst);
        return -1;

    } else if (pid == 0) {
        int result = send_all_shm(mem->self, fd_dst);
        close(fd_dst);
        dtor_shm(mem);

        if (result == 0) exit(0);
        perror("recieve failed (shm)\n");
        exit(-1);

    } else {
        int result = recieve_all_shm(mem->self, fd_src);
        waitpid(pid, NULL, 0);
        close(fd_src);

        if (result == 0) return 0;
        perror("send failed (shm)\n");
        return -1;
    }
}
