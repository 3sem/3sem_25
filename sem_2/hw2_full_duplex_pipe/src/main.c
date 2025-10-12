#include "pipe.h"

int main (int argc, char *argv[])
{
    Pipe *pipe = dup_pipe_ctor();

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Fork filed");
    }
    else if (pid == 0)
    {
        child_echo_back(pipe);
    } 
    else 
    {
        parent_echo_send(pipe, argv[1]);
    }

    dup_pipe_dtor(pipe);
    return 0;
}

