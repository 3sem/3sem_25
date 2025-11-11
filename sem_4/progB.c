#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int n = atoi(argv[1]);
    int N = atoi(argv[2]);

    int fd = shm_open("/mc_shared", O_RDWR, 0666);
    int *shared = mmap(NULL, n * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int total = 0;
    for (int i = 0; i < n; i++) total += shared[i];

    double integral = (double)total / N;
    printf("Integral = %.6f\n", integral);
    printf("Exact value = 0.5\n");

    munmap(shared, n * sizeof(int));
    close(fd);
    return 0;
}
