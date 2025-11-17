#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "fifo_send.h"
#include "common.h"

int recieve_fifo (Fifo *fifo, int fd, int size);
int send_fifo    (Fifo *fifo, int fd, int size);
int open_fifo  (Fifo *fifo);
int close_fifo (Fifo *fifo);

int assign_commands_fifo(Op_fifo *self) {
    if (!self) return -1;

    self->open  = open_fifo;
    self->close = close_fifo;

    return 0;
}

Fifo *ctor_fifo() {
    Fifo *fifo;

    CALLOC_SELF(fifo);
    if (assign_commands_fifo(&fifo->op) == -1) {
        free(fifo);
        perror("assign commands error\n");
        return 0;
    }

    CALLOC_DTOR(fifo->buffer, FIFO_BUFFER_SIZE, fifo, dtor_fifo, 0);
    CALLOC_DTOR(fifo->name,   FIFO_NAME_SIZE,   fifo, dtor_fifo, 0);
    sprintf(fifo->name, FIFO_NAME_PREFIX "pid_%d_addr_%p", getpid(), fifo);
    mkfifo(fifo->name, 0666);

    return fifo;
}

int dtor_fifo(Fifo *fifo) {
    if (!fifo) return 0;

    if (fifo->op.close(fifo) == -1) {
        perror("fifo close failed\n");
    }

    unlink(fifo->name);
    if (fifo->name) {
        memset(fifo->name, 0, FIFO_NAME_SIZE);
        free(fifo->name);
    }
    if (fifo->buffer) {
        memset(fifo->buffer, 0, FIFO_BUFFER_SIZE);
        free(fifo->buffer);
    }

    SET_ZERO(fifo);
    free(fifo);

    return 0;
}

int send_file_fifo (Fifo *fifo, const char *src_file, const char *dst_file) {
    if (!fifo) {
        perror("no fifo object\n");
        return -1;
    }

    fifo->op.open(fifo);

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

    int size = get_file_size(src_file);

    pid_t pid1 = fork();

    if (pid1 == -1) {
        close(fd_src);
        close(fd_dst);
        perror("fork recieve failed (fifo)\n");
        return -1;

    } else if (pid1 == 0) {
        close(fd_src);
        int result = send_fifo(fifo, fd_dst, size);
        dtor_fifo(fifo);
        close(fd_dst);

        if (result == 0) exit(0);
        perror("send failed (fifo)\n");
        exit(-1);
    }
    pid_t pid2 = fork();
    if (pid2 == -1) {
        close(fd_src);
        close(fd_dst);
        kill(pid1, SIGINT);
        perror("fork send failed (fifo)\n");
        return -1;

    } else if (pid2 == 0) {
        close(fd_dst);
        int result = recieve_fifo(fifo, fd_src, size);
        dtor_fifo(fifo);
        close(fd_src);

        if (result == 0) exit(0);
        perror("recieve failed (fifo)\n");
        exit(-1);
    }
    close(fd_dst);
    close(fd_src);
    waitpid(pid2, NULL, 0);
    waitpid(pid1, NULL, 0);
    return 0;
}

int recieve_fifo(Fifo *fifo, int fd, int size) {
    if (!fifo) {
        perror("no fifo object\n");
        return -1;
    }

    int fd_in  = fd;
    int fd_out = fifo->fd;
    SET_BLOCK(fd_out);
    SET_BLOCK(fd_in);

    printf("\x1b[34m""recieve started\n""\x1b[0m");
    while (size > 0) {
        if ((fifo->size = read(fd_in, fifo->buffer, FIFO_BUFFER_SIZE)) == -1) {
            printf("\x1b[34m""ERROR IN read recieve; errno = %d\n""\x1b[0m", errno);
            return -1;
        }
        size -= fifo->size;
        printf("\x1b[34m""sended = %d\n""\x1b[0m", fifo->size);
        if (write(fd_out, fifo->buffer, fifo->size) == -1) {
            printf("\x1b[34m""ERROR IN write recieve; errno = %d\n""\x1b[0m", errno);
            return -1;
        }
        fifo->size = 0;
    }
    printf("\033[4m""\x1b[34m""recieve ended\n""\x1b[0m""\033[0m");

    return 0;
}
int send_fifo(Fifo *fifo, int fd, int size) {
    if (!fifo) {
        perror("no fifo object\n");
        return -1;
    }

    int fd_in  = fifo->fd;
    int fd_out = fd;
    SET_BLOCK(fd_out);
    SET_BLOCK(fd_in);

    printf("\x1b[32m""send started\n""\x1b[0m");
    while (size > 0) {
        if ((fifo->size = read(fd_in, fifo->buffer, FIFO_BUFFER_SIZE)) == -1) {
            printf("\x1b[32m""ERROR IN read send; errno = %d\n""\x1b[0m", errno);
            return -1;
        }
        size -= fifo->size;
        printf("\x1b[32m""sended = %d\n""\x1b[0m", fifo->size);
        if (write(fd_out, fifo->buffer, fifo->size) == -1) {
            printf("\x1b[32m""ERROR IN write send; errno = %d\n""\x1b[0m", errno);
            return -1;
        }
        fifo->size = 0;
    };
    printf("\033[4m""\x1b[32m""send ended\n""\x1b[32m""\033[0m");

    return 0;
}

int open_fifo  (Fifo *fifo) {
    if (!fifo) {
        perror("no fifo object\n");
        return -1;
    }
    if (!fifo->name) {
        perror("no fifo name\n");
        return -1;
    }
    if (fifo->fd > 0) {
        printf("fifo already opened or broken\n");
        return 0;
    }

    fifo->fd = open(fifo->name, O_RDWR);

    return 0;
}
int close_fifo (Fifo *fifo) {
    if (!fifo) {
        perror("no fifo object\n");
        return -1;
    }
    if (!fifo->name) {
        perror("no fifo name\n");
    }

    if (fifo->fd > 0)  close(fifo->fd);

    return 0;
}

