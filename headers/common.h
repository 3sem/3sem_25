#ifndef COMMON
#define COMMON

#include<memory.h>

#define CALLOC_ERR(x, size, err)      \
if (!(x = calloc(size, sizeof(*x)))) {\
    perror("calloc" #x);              \
    return err;                       \
}

#define CALLOC_CTOR(x, size, this, dtor, err)\
if (!(x = calloc(size, sizeof(*x)))) {       \
    perror("calloc" #x);                     \
    if (dtor(this)) exit(1);                 \
    return err;                              \
}

int write_result(int fd_in);

#define SET_ZERO(x) memset(x, 0, sizeof(*x))

#endif