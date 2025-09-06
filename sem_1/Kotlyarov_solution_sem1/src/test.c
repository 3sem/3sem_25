#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main() {

    pid_t pid = fork();
    if (pid == 0) {
    //child
        //printf("Child: %d\n", getpid());
        execlp("ls", "-l", (char*) NULL);
    }
    else {

        printf("Parent: %d of child %d\n", getpid(), pid);
    }
    return 0;
}
