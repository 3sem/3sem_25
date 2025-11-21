#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Test program PID: %d\n", getpid());
    
    while(1) {
        sleep(10);
    }
    return 0;
}