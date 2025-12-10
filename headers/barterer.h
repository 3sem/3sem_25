#ifndef BARTERER
#define BARTERER

#include <signal.h>
// #include <asm-generic/signal.h>
// #include <asm-generic/siginfo.h>
// #include <asm-generic/signal-defs.h>

typedef void (*handler_t)(int, siginfo_t *, void *);
typedef struct barter_data barter_t;
typedef struct shm_data shm_t;
typedef struct sig_data sig_data_t;

#define SHM_NAME "barter_shm"
#define SHM_SIZE 4096

#define SIG_SWITCH SIGRTMAX
#define SIG_FLAG SIGRTMIN

struct sig_data {
    struct sigaction sa;
    sigset_t set;
};

struct barter_data {
    shm_t* shm_ptr;

    const char* shm_name;
    int shm_fd;

    sig_data_t sig;
};

struct shm_data {
    int pid_snd;
    int pid_rcv;
    int size;
};

barter_t *barter_ctor();
int barter_dtor(barter_t* data);

int send_file(barter_t* data, const char * src_file_name);
int recieve_file(barter_t* data, const char * dst_file_name);

#endif
