#include "handlers.h"

struct Shared_data* sh_data;

int main(int argc, char *argv[]) {

    if (argc != 3) {

        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(1);
    }

    FILE* input_file = freopen(argv[1], "r", stdin);
    if (!input_file) {

        perror("freopen");
        return 1;
    }

    FILE* output_file = freopen(argv[2], "w", stdout);
    if (!output_file) {

        perror("freopen");
        return 1;
    }

    key_t shm_key = ftok(".", 'M');
    if (shm_key == -1) {

        perror("ftok (shm)");
        fclose(input_file);
        return 1;
    }

    int shmid = shmget(shm_key, Shm_size, IPC_CREAT | 0666);
    if (shmid == -1) {

        perror("shmget");
        fclose(input_file);
        return 1;
    }

    char* shm_ptr = (char*) shmat(shmid, NULL, 0);
    if (shm_ptr == (void*) -1) {

        perror("shmat");
        fclose(input_file);
        return 1;
    }

    sh_data = (struct Shared_data*) shm_ptr;
    sh_data->buffer = shm_ptr + sizeof(struct Shared_data);
    sh_data->buf_size = Shm_size;
    sh_data->ppid = getpid();
    sh_data->producer_ended = NOT_ENDED;
    sh_data->consumer_ended = NOT_ENDED;

    struct sigaction sa = {.sa_sigaction = producer_handler, .sa_flags = SA_SIGINFO};
    sigaction(SIG_PROD, &sa, NULL);
    sa.sa_sigaction  = consumer_handler;
    sigaction(SIG_CONS, &sa, NULL);
    sa.sa_flags = 0;
    sa.sa_handler = sig_exit_handler;
    sigaction(SIG_EXIT, &sa, NULL);

    struct timespec start, end;
    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        return -1;
    }

    if (pid == 0) {

        union sigval sv = {.sival_int = 0};
        for (int i = 0; i < CHUNKS; i++) {
            
            sv.sival_int = i;
            sh_data->producer_chunks[i] = sh_data->buffer + i * Chunk_size;
            sh_data->consumer_chunks[i].chunk = sh_data->buffer + i * Chunk_size;
            sigqueue(sh_data->ppid, SIG_PROD, sv);
        }

        //pause();

        while(sh_data->producer_ended == NOT_ENDED);
        //DEBUG_PRINTF("\nConsumer ended with exit code: %d\n", sh_data->consumer_ended);
        int exit_code = sh_data->consumer_ended;
        if (exit_code != CONS_END_FAIL)
            exit_code = CONS_END_SUCC;
        shmdt(shm_ptr);
        //shmctl(shmid, IPC_RMID, NULL);
        exit(exit_code);
    }

    else {

        clock_gettime(CLOCK_MONOTONIC, &start);
        sh_data->pid = pid;
        while(sh_data->producer_ended == NOT_ENDED && sh_data->consumer_ended == NOT_ENDED);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) +
                            (end.tv_nsec - start.tv_nsec) / 1e9;
        fprintf(stderr, "%s: Time consumed = %.6f seconds\n", argv[0], time_taken);
        //DEBUG_PRINTF("\nProducer ended with exit code: %d\n", sh_data->producer_ended);
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        return 0;
    }
}
