#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>

#define BUF 65536

typedef struct DuplexPipe {
    int p2c[2];
    int c2p[2];
    int child;

    int (*init)(struct DuplexPipe *self);
    int (*echo)(struct DuplexPipe *self, const char *infile, const char *outfile);
    void (*destroy)(struct DuplexPipe *self);
} DuplexPipe;

static void child_loop(int rd, int wr) {
    char *buf = malloc(BUF);
    int n;
    while ((n = read(rd, buf, BUF)) > 0) {
        write(wr, buf, n);
    }
    free(buf);
    exit(0);
}

static int echo(DuplexPipe *self, const char *infile, const char *outfile) {
    int in = open(infile, 0);
    int out = open(outfile, O_CREAT|O_TRUNC|O_WRONLY, 0644);

    close(self->p2c[0]);
    close(self->c2p[1]);
    int p2c = self->p2c[1];
    int c2p = self->c2p[0];

    char *buf = malloc(BUF);
    long long total = 0;

    struct timespec t0, t1;
    clock_gettime(0, &t0);

    int n;
    while ((n = read(in, buf, BUF)) > 0) {
        write(p2c, buf, n);
        int m = 0;
        while (m < n) {
            int r = read(c2p, buf + m, n - m);
            write(out, buf + m, r);
            m += r;
        }
        total += n;
    }

    clock_gettime(0, &t1);
    double secs = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9;
    double mb = total / (1024.0*1024.0);

    printf("Size: %.2f MB\n", mb);
    printf("Time: %.3f s\n", secs);
    printf("Speed: %.2f MB/s\n", mb/secs);

    free(buf);
    close(in);
    close(out);
    return 0;
}

static int duplex_init(DuplexPipe *self) {
    pipe(self->p2c);
    pipe(self->c2p);
    self->child = fork();
    if (self->child == 0) {
        close(self->p2c[1]);
        close(self->c2p[0]);
        child_loop(self->p2c[0], self->c2p[1]);
        exit(0);
    }
    return 0;
}

static void duplex_destroy(DuplexPipe *self) {
    close(self->p2c[1]);
    close(self->c2p[0]);
    wait(NULL);
}

DuplexPipe *DuplexPipe_create() {
    DuplexPipe *d = malloc(sizeof(DuplexPipe));
    d->init = duplex_init;
    d->echo = echo;
    d->destroy = duplex_destroy;
    return d;
}


int main(int argc, char **argv) {
    DuplexPipe *d = DuplexPipe_create();
    d->init(d);
    d->echo(d, argv[1], argv[2]);
    d->destroy(d);
    free(d);
    return 0;
}
