#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int main() {

    pid_t pid = fork();
    if (pid == 0) {
    //child
        //printf("Child: %d\n", getpid());
        sleep(1);
        //execlp("ls", "-l", (char*) NULL);
        exit(42);
    }
    else {
        
        int status = 0;
        printf("Parent: %d of child %d\n", getpid(), pid);
        waitpid(pid, &status, 0);
        int result = WIFEXITED(status);
        if (result != 0) {
            
            printf("Process %d is exited with exit code %d", pid, WEXITSTATUS(status));
        }
        else {

            printf("Process %d is not exited normally", pid);
        }
    }
    return 0;
}
