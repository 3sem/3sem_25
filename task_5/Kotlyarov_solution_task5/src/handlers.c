#include "handlers.h"

extern struct Shared_data* sh_data;

void sig_exit_handler(int sig) {

    sh_data->consumer_ended = CONS_END_SUCC;
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_CONS, &sa, NULL);
    DEBUG_PRINTF("SIG_EXIT was sent\n");
}

void producer_handler(int sig, siginfo_t *info, void *ucontext) {

    int chunk_num = info->si_value.sival_int;

    ssize_t curr_size = read(0, sh_data->producer_chunks[chunk_num], Chunk_size);
    while (curr_size == 0 && sh_data->bytes_read < sh_data->file_size && sh_data->attempts < Max_attempts) {

        curr_size = read(0, sh_data->producer_chunks[chunk_num], Chunk_size);
        sh_data->attempts++;
    }
    
    if (curr_size > 0) {

        union sigval sv = {.sival_int = chunk_num};
        sh_data->consumer_chunks[chunk_num].chunk_size = curr_size;
        sigqueue(sh_data->pid, SIG_CONS, sv);
    }

    else {

        /*if (curr_size < 0) {

            DEBUG_PRINTF("ERROR: read\n");
            sh_data->producer_ended = PROD_END_FAIL;
        }*/

        if (sh_data->bytes_read == sh_data->file_size) {

            sh_data->producer_ended = PROD_END_SUCC;
        }

        else {

            DEBUG_PRINTF("ERROR: read\n");
            sh_data->producer_ended = PROD_END_FAIL;
        }

        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sigaction(SIG_PROD, &sa, NULL);
        //kill(sh_data->pid, SIG_EXIT);
        return;
    }

    sh_data->bytes_read += curr_size;
}

void consumer_handler(int sig, siginfo_t *info, void *ucontext) {

    int chunk_num = info->si_value.sival_int;

    size_t bytes_written_total = 0;
    size_t bytes_to_write = sh_data->consumer_chunks[chunk_num].chunk_size;
    const char *buf_ptr = sh_data->consumer_chunks[chunk_num].chunk;

    while (bytes_written_total < bytes_to_write) {

        ssize_t curr_size = write(1, buf_ptr + bytes_written_total, bytes_to_write - bytes_written_total);

        if (curr_size == -1) {

            perror("write");
            sh_data->consumer_ended = CONS_END_FAIL;
            struct sigaction sa;
            sa.sa_handler = SIG_IGN;
            sigemptyset(&sa.sa_mask);
            sigaction(SIG_CONS, &sa, NULL);
            return;
        }

        bytes_written_total += curr_size;
    }

    union sigval sv = {.sival_int = chunk_num};
    sigqueue(sh_data->pid, SIG_PROD, sv);
}
