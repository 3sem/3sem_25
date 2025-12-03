#ifndef COMMON
#define COMMON

#include<memory.h>

#define CALLOC_SELF(x)                           \
if (!(x = calloc(1, sizeof(*x)))) {              \
    perror("calloc " #x "error");                \
    return 0;                                    \
}

#define CALLOC_ERR(x, size, err)                 \
if (!(x = calloc(size, sizeof(*x)))) {           \
    perror("calloc " #x " error");               \
    return err;                                  \
}

#define CALLOC_DTOR(x, size, this, dtor, err)    \
if (!(x = calloc(size, sizeof(*x)))) {           \
    perror("calloc " #x " error");               \
    if (dtor(this)) exit(1);                     \
    return err;                                  \
}
#define SET_NONBLOCK(fd)                         \
{                                                \
    int flags = fcntl(fd, F_GETFL, 0);           \
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);  \
}
#define SET_BLOCK(fd)                            \
{                                                \
    int flags = fcntl(fd, F_GETFL, 0);           \
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK); \
}

#define ERR_R(x, ...) fprintf(stderr, "\033[31m" x "\033[0m" __VA_OPT__(,) __VA_ARGS__)
#define ERR_Y(x, ...) fprintf(stderr, "\033[33m" x "\033[0m" __VA_OPT__(,) __VA_ARGS__)
#define ERR_G(x, ...) fprintf(stderr, "\033[32m" x "\033[0m" __VA_OPT__(,) __VA_ARGS__)

#define SET_ZERO(x) memset(x, 0, sizeof(*x))

int write_result(int fd_in);
int get_file_size(const char *file_name);

#endif
