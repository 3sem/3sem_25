#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

int main() {
    
    int fd = 0;
    char* fifo_path = "myfifio";
    mkfifo(fifo_path, 0666); 

    char buf[4096];

    fd = open(fifo_path, O_WRONLY);

    while (1) {

        scanf("%s", buf);
        write(fd, buf strlen(buf) + 1);

    }
    return 0;
}

