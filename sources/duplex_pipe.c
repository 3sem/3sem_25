#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "duplex_pipe.h"
#include "common.h"

int open_Pipe  (Pipe *pipe_);
int close_Pipe (Pipe *pipe_);
int recieve    (Pipe *pipe_); 
int send       (Pipe *pipe_);

int assign_commands(Op_pipe *self) {
    if (!self) return -1;

    self->open  = open_Pipe;
    self->close = close_Pipe;
    self->rcv   = recieve;
    self->snd   = send;

    return 0;
}

int dtor_Pipe(Pipe *pipe_) {
    if (!pipe_) return 0;

    if (pipe_->data) free(pipe_->data);
    if (pipe_->status != BROKEN && pipe_->status != UNUSED) {
        pipe_->actions.close(pipe_);
    }

    SET_ZERO(pipe_);
    free(pipe_);

    return 0;
}
Pipe *ctor_Pipe() {
    Pipe *pipe_ = malloc(sizeof(Pipe));

    pipe_->status = UNUSED;
    CALLOC_CTOR(pipe_->data, BUF_SZ, pipe_, dtor_Pipe, 0);
    assign_commands(&pipe_->actions);
    pipe_->len = 0;
    pipe_->fd_read = pipe_->fd_write = 0;

    return pipe_;
}

int open_Pipe(Pipe *pipe_) {
    if (!pipe_) {
        perror("no pipe to open\n");
        return -1;
    }

    switch (pipe_->status) {
    case BROKEN:
        {
        perror("can not open broken pipe\n");
        return -1;
        }
    case DIRECT:
        {
        perror("can not open already direct pipe\n");
        return -1;
        }
    case BACK:
        {
        perror("can not open already back pipe\n");
        return -1;
        }
    default:
        break;
    }

    int fd_direct[2];
    int fd_back[2];
    pipe(fd_direct);
    pipe(fd_back);
    
    int pid;
    pid = fork();

    if (pid == -1) {
        close(fd_back[0]);
        close(fd_back[1]);
        close(fd_direct[0]);
        close(fd_direct[1]);

        perror("open pipe: fork error");
        return -1;
    }
    else if (pid == 0) {
        pipe_->status = BACK;
        pipe_->fd_read  = fd_direct[0];
        pipe_->fd_write = fd_back[1];

        close(fd_direct[1]);
        close(fd_back[0]);
    }
    else {
        printf("Created direct %d and back %d\n", getpid(), pid);

        pipe_->status = DIRECT;
        pipe_->fd_read  = fd_back[0];
        pipe_->fd_write = fd_direct[1];

        close(fd_back[1]);
        close(fd_direct[0]);
    }

    int flags = fcntl(pipe_->fd_read, F_GETFL, 0);
    fcntl(pipe_->fd_read, F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(pipe_->fd_write, F_GETFL, 0);
    fcntl(pipe_->fd_write, F_SETFL, flags & ~O_NONBLOCK);
    return pid;
}
int close_Pipe(Pipe *pipe_) {
    if (!pipe_) {
        perror("no pipe to close\n");
        return -1;
    }

    switch (pipe_->status) {
    case BROKEN:
        {
        perror("can not close broken pipe\n");
        return -1;
        }
    case UNUSED:
        {
        perror("can not close unused pipe\n");
        return -1;
        }
    default:
        break;
    }

    close(pipe_->fd_read);
    close(pipe_->fd_write);

    printf("closed %s pipe in process %d\n", pipe_->status == DIRECT ? "direct" : "back", getpid());

    pipe_->status = UNUSED;

    return 0;
}

int recieve (Pipe *pipe_) {
    if (!pipe_) {
        perror("recieve: no pipe\n");
        return -1;
    }

    int res = read(pipe_->fd_read, pipe_->data, BUF_SZ);
    if (res != -1) pipe_->len = res;
    else {
        int flags = fcntl(pipe_->fd_read, F_GETFL, 0);

        fcntl(pipe_->fd_read, F_SETFL, flags & ~O_NONBLOCK);
        res = read(pipe_->fd_read, pipe_->data, 1);
        fcntl(pipe_->fd_read, F_SETFL, flags | O_NONBLOCK);
        
        if (res != -1) pipe_->len = res;
    }
    
    // printf("recieved %lu bytes pid = %d\n", pipe_->len, getpid());

    return res;
}
int send (Pipe *pipe_) {
    if (!pipe_) {
        perror("send: no pipe\n");
        return -1;
    }
    
    size_t start_pos = 0;

    while(start_pos != pipe_->len) {
        start_pos += write(pipe_->fd_write, pipe_->data + start_pos, pipe_->len - start_pos);
    }

    pipe_->len = 0;

    // printf("sent %lu bytes pid = %d\n", start_pos, getpid());

    return start_pos;
}

int send_file(Pipe *pipe_, const char *file_name) {
    if (!pipe_) {
        perror("no pipe to send\n");
        return -1;
    }
    if (!file_name) {
        perror("no file to send\n");
        return -1;
    }

    int file_fd = open(file_name, O_RDONLY);
    
    int stored_read_fd = pipe_->fd_read;
    pipe_->fd_read = file_fd;

    while (pipe_->actions.rcv(pipe_)) pipe_->actions.snd(pipe_);

    int eof = EOF;
    write(pipe_->fd_write, &eof, 1);

    pipe_->fd_read = stored_read_fd;
    close(file_fd);
    return 0;
}

int recieve_file(Pipe *pipe_, const char *file_name) {
    if (!pipe_) {
        perror("no pipe to send\n");
        return -1;
    }
    if (!file_name) {
        perror("no file to send\n");
        return -1;
    }

    int file_fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);\

    int stored_write_fd = pipe_->fd_write;
    pipe_->fd_write = file_fd;

    int eof_flag = 0;
    int i = 0;

    while (!eof_flag) {
        i++;
        pipe_->actions.rcv(pipe_);
        if(pipe_->len && pipe_->data[pipe_->len - 1] == EOF) {
            eof_flag = 1;
            pipe_->len--;
        }
        pipe_->actions.snd(pipe_);
    }

    pipe_->fd_write = stored_write_fd;
    close(file_fd);
    return 0;
}

