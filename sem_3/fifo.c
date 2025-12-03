#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>

void md5check (char* infile, char *outfile) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),"md5sum %s > %s", outfile, infile);
    system(cmd);
}

int main(int argc, char **argv){
    char *infile = argv[1];
    char *outfile = argv[2];
    int buf = atoi(argv[3]);
    char *path = "/tmp/fifo_test";

    mkfifo(path, 0666);

    pid_t pid = fork();
    struct timespec t0, t1;
    if (pid == 0) {
        // reader
        int fd = open(path, O_RDONLY);
        FILE *out = fopen(outfile, "w");
        char *b = malloc(buf);
        int n;

        while ((n = read(fd, b, buf)) > 0) {
            fwrite(b, 1, n, out);
        }

        fclose(out);
        close(fd);

        md5check("/tmp/md5_recv.txt", outfile);

        exit(0);
    } else {
        // writer
        int fd = open(path, O_WRONLY);
        FILE *in = fopen(infile, "r");

        clock_gettime(CLOCK_MONOTONIC, &t0);

        char *b = malloc(buf);
        int n;
        while ((n = fread(b, 1, buf, in))>0) {
            write(fd, b, n);
        }

        close(fd);
        fclose(in);
        wait(NULL);

        clock_gettime(CLOCK_MONOTONIC, &t1);
        double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

        md5check("/tmp/md5_src.txt", infile);

        printf("FIFO elapsed: %.6f\n", elapsed);
        system("echo 'src md5:'; cat /tmp/md5_src.txt");
        system("echo 'recv md5:'; cat /tmp/md5_recv.txt");
        unlink(path);
    }
    return 0;
}
