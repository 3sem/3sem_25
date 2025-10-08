#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "duplex_pipe.h"


int main() {
    Pipe *pipe_ = ctor_Pipe();

    int pid = pipe_->actions.open(pipe_);
    int res;

    if (pipe_->status == DIRECT) {
        res = send_file    (pipe_, "text/text");
        printf("Send complited by %d\n", getpid());
        recieve_file (pipe_, "text/text_dup2");
        printf("Recieve complited by %d\n", getpid());
        waitpid(pid, NULL, 0);
    }
    else {
        res = recieve_file (pipe_, "text/text_tmp");
        printf("Recieve complited by %d %d\n", getpid(), res);
        send_file    (pipe_, "text/text_tmp");
        printf("Send complited by %d\n", getpid());
    }

    pipe_->actions.close(pipe_);

    dtor_Pipe(pipe_);
    
    return 0;
}