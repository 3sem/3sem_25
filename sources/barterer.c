#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "barterer.h"
#include "common.h"

enum SIG_INFO_T {
    TERMINATE     = 0,
    START_WORK    = 1,
    FINISH_WORK   = 2,
    RISE_SENDER   = 3,
    RISE_RECIEVER = 4,
};

static int init_signal_data(sig_data_t *sig);
static void handler(int sig, siginfo_t *info, void *ucontext);
static int wait_for_participant(barter_t* data, int* self_pid, int* other_pid);


barter_t *barter_ctor() {
    barter_t* self;
    CALLOC_SELF(self);

    self->shm_name = SHM_NAME;
    self->shm_fd = shm_open(self->shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(self->shm_fd, SHM_SIZE);
    self->shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, self->shm_fd, 0);

    init_signal_data(&self->sig);

    ERR_G("barter %d created\n", getpid());

    return self;
}
int barter_dtor(barter_t* data) {
    if (!data) {
        ERR_Y("no data object to destruct\n");
        return 0;
    }

    // munmap(data->shm_ptr, SHM_SIZE);
    shm_unlink(data->shm_name);

    SET_ZERO(data);
    free(data);

    ERR_G("barter %d destructed\n", getpid());

    return 0;
}

int send_file(barter_t* data, const char * src_file_name) {
    if (!data) {
        ERR_R("no data object\n");
        return 1;
    }
    if (!src_file_name) {
        ERR_R("no data src file\n");
        return 1;
    }

    sigprocmask(SIG_BLOCK, &data->sig.set, NULL);

    if (wait_for_participant(data, &data->shm_ptr->pid_snd, &data->shm_ptr->pid_rcv)) {
        ERR_R("error while waiting reciever\n");
        return 1;
    }
    ERR_Y("PASSED\n");

    int fd_src = open(src_file_name, O_RDONLY | O_CREAT, 0666);
    if (fd_src == -1) {
        ERR_R("can not open source file\n");
        kill(data->shm_ptr->pid_rcv, SIGINT);
        return 1;
    }
    SET_BLOCK(fd_src);

    int sig;

    union sigval sv = {.sival_int = RISE_RECIEVER};

    while ((data->shm_ptr->size = read(fd_src, data->shm_ptr + 1, SHM_SIZE - sizeof(shm_t)))) {
        // ERR_Y("%d sended\n", data->shm_ptr->size);
        // ERR_R("ERRNO = %d\n", errno);
        sigqueue(data->shm_ptr->pid_rcv, SIG_SWITCH, sv);
        sigwait(&data->sig.set, &sig);
        if (sig != SIG_SWITCH) {
            ERR_R("recieved %d signal %d expected\n", sig, SIG_SWITCH);
            kill(data->shm_ptr->pid_rcv, SIGINT);
            return 1;
        }
    }

    sv.sival_int = FINISH_WORK;
    sigqueue(data->shm_ptr->pid_rcv, SIG_FLAG, sv);

    sigprocmask(SIG_UNBLOCK, &data->sig.set, NULL);

    ERR_G("sender finished\n");

    return 0;
}
int recieve_file(barter_t* data, const char * dst_file_name) {
    if (!data) {
        ERR_R("no data object\n");
        return 1;
    }
    if (!dst_file_name) {
        ERR_R("no data dst file\n");
        return 1;
    }

    sigprocmask(SIG_BLOCK, &data->sig.set, NULL);

    if (wait_for_participant(data, &data->shm_ptr->pid_rcv, &data->shm_ptr->pid_snd)) {
        ERR_R("error while waiting sender\n");
        return 1;
    }
    ERR_Y("PASSED\n");

    int fd_dst = open(dst_file_name, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (fd_dst == -1) {
        ERR_R("can not open desteny file\n");
        kill(data->shm_ptr->pid_snd, SIGINT);
        return 1;
    }
    SET_BLOCK(fd_dst);

    int sig;

    union sigval sv = {.sival_int = RISE_SENDER};

    while (1) {
        sigwait(&data->sig.set, &sig);
        if (sig == SIG_FLAG) break;
        if (data->shm_ptr->size == -1) {
            ERR_R("read error\n");
            kill(data->shm_ptr->pid_snd, SIGINT);
            return 1;
        }
        write(fd_dst, data->shm_ptr + 1, data->shm_ptr->size);
        // ERR_Y("%d recieved\n", data->shm_ptr->size);
        sigqueue(data->shm_ptr->pid_snd, SIG_SWITCH, sv);
    }

    sigprocmask(SIG_UNBLOCK, &data->sig.set, NULL);

    ERR_G("reciever finished\n");

    return 0;
}

static int init_signal_data(sig_data_t *sig) {
    if (!sig) {
        ERR_R("no sig object\n");
        return 1;
    }
    // sig->sa.sa_handler = handler;
    sig->sa.sa_flags = SA_SIGINFO;
    sig->sa.sa_sigaction = handler;

    sigemptyset(&sig->set);
    sigaddset(&sig->set, SIGRTMIN);
    sigaddset(&sig->set, SIGRTMAX);
    sigaction(SIGRTMIN, &sig->sa, NULL);
    sigaction(SIGRTMAX, &sig->sa, NULL);

    return 0;
}

static int wait_for_participant(barter_t* data, int* self_pid, int* other_pid) {
    if (!data) {
        ERR_R("no data object\n");
        return 1;
    }
    if (!self_pid || !other_pid) {
        ERR_R("sone pid field do not exist\n");
        return 1;
    }

    union sigval sv = {.sival_int = START_WORK};

    // sigprocmask(SIG_BLOCK, &data->sig.set, NULL);

    if (*self_pid) {
        ERR_R("previos pid not zero for %d %d\n", getpid(), *self_pid);
        return 1;
    }

    *self_pid = getpid();

    int sig;

    if (*other_pid) {
        sigqueue(*other_pid, SIG_FLAG, sv);
        sigwait(&data->sig.set, &sig);
        if (sig != SIG_FLAG) {
            ERR_R("reciarved signal %d expected %d\n", sig, SIG_FLAG);
            return 1;
        }
    }
    else {
        sigwait(&data->sig.set, &sig);
        if (sig != SIG_FLAG) {
            ERR_R("recieved signal %d expected %d\n", sig, SIG_FLAG);
            return 1;
        }
        if (*other_pid == 0) {
            ERR_R("recieved expected signal but others pid still 0\n");
            return 1;
        }
        sigqueue(*other_pid, SIG_FLAG, sv);
    }

    // sigprocmask(SIG_UNBLOCK, &data->sig.set, NULL);
    return 0;
}

static void handler(int sig, siginfo_t *info, void *ucontext) {
    if (sig == SIG_FLAG && info->si_value.sival_int == START_WORK) {
        ERR_G("Process %d recieve start signal\n", getpid());
        return;
    }
    if (sig == SIG_FLAG && info->si_value.sival_int == FINISH_WORK) {
        ERR_G("Process %d recieve finish signal\n", getpid());
        return;
    }
    if (sig == SIG_SWITCH && info->si_value.sival_int == RISE_SENDER)   return;
    if (sig == SIG_SWITCH && info->si_value.sival_int == RISE_RECIEVER) return;
    if (info->si_value.sival_int == TERMINATE) {
        ERR_R("Termenating process\n");
        exit(1);
    }
    ERR_Y("Unknown signal type sig = %d info = %d\n", sig, info->si_value.sival_int);
    return;
}
