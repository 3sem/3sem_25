#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

#include "duplex_pipe.h"


int main() {
    Pipe *pipe_ = ctor_Pipe();

    int pid = pipe_->actions.open(pipe_);
    int res;

    time_t time_start = time(NULL);

    if (pipe_->status == DIRECT) {
        res = send_file    (pipe_, "text/text");
        printf("Send complited by %d\n", getpid());
        recieve_file (pipe_, "text/text_dup");
        printf("Recieve complited by %d\n", getpid());
        waitpid(pid, NULL, 0);
    }
    else {
        res = recieve_file (pipe_, "text/text_tmp");
        printf("Recieve complited by %d %d\n", getpid(), res);
        send_file    (pipe_, "text/text_tmp");
        printf("Send complited by %d\n", getpid());
    }

    time_t time_end = time(NULL);

    double prog_time = difftime(time_start, time_end);

    pipe_->actions.close(pipe_);

    dtor_Pipe(pipe_);

    printf("\x1b[32m" "\nTime use to sent -> & <-: %lg\n" "\x1b[0m", prog_time);
    
    return 0;
}