
#include <sys/types.h>
#include <time.h>

#define BUF_SIZE 1048576

struct shmseg {
    size_t bytes;
    int ready;
    int ended;
    double start_time;
    char buf[BUF_SIZE];
};

static inline double now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

