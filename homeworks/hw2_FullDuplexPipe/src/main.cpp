#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "FullDuplexPipe.hpp"

int create_pipe(pPipe* Pipe, size_t buf_size);
int destroy_pipe(pPipe* Pipe);

static int child_run(pPipe* Pipe);
static int parent_run(pPipe* Pipe);

int main() {

    pPipe Pipe = {};
    if (create_pipe(&Pipe, BUF_SIZE))
        return 1;

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        child_run(&Pipe);
        destroy_pipe(&Pipe);
        return 1;
    }
    else {
        if (parent_run(&Pipe))
            return 1;
        destroy_pipe(&Pipe);
        wait(NULL);
    }

    return 0;
}

static int parent_run(pPipe* Pipe) {

    int file = open(FILE_NAME, O_RDONLY | 0644);
    if (file < 0) {
        if (errno == ENOENT) {
            system(FILE_CREATE);
            file = open(FILE_NAME, O_RDONLY | 0644);
        }
        else {
            perror("opening file failed");
            return 1;
        }
    }

    int new_file = open(FILE_NEW FILE_NAME, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (new_file < 0) {
        perror("opening new file failed");
        return 1;
    }

    close(Pipe->fd_direct[READ_PIPE]);
    close(Pipe->fd_back[WRITE_PIPE]);

    dup2(Pipe->fd_direct[WRITE_PIPE], STDOUT_FILENO);
    close(Pipe->fd_direct[WRITE_PIPE]);

    dup2(Pipe->fd_back[READ_PIPE], STDIN_FILENO);
    close(Pipe->fd_back[READ_PIPE]);

    while (PIPE_RCV(file) > 0) {
        PIPE_SND(STDOUT_FILENO);
        PIPE_RCV(STDIN_FILENO);
        PIPE_SND(new_file);
    }

    close(file);
    close(new_file);
    fclose(stdout);

    return 0;
}

static int child_run(pPipe* Pipe) {

    close(Pipe->fd_direct[WRITE_PIPE]);
    close(Pipe->fd_back[READ_PIPE]);

    dup2(Pipe->fd_direct[READ_PIPE], STDIN_FILENO);
    close(Pipe->fd_direct[READ_PIPE]);

    dup2(Pipe->fd_back[WRITE_PIPE], STDOUT_FILENO);
    close(Pipe->fd_back[WRITE_PIPE]);

    while (PIPE_RCV(STDIN_FILENO) > 0)
        PIPE_SND(STDOUT_FILENO);

    return 0;
}
