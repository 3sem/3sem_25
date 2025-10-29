#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

int write_result(int fd_in) {
    pid_t pid = 0;
    int status = 0;

    if ((pid = fork()) == -1) {
        perror("fork failed\n");
        return 1;
    } else if (pid == 0) {
        dup2(fd_in, STDIN_FILENO);
        execlp("cat", "cat", (char *)NULL);

        perror("execlp failed\n");
        return 1;
    }
    waitpid(pid, &status, 0);

    return 0;
}
