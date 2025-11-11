#define _POSIX_C_SOURCE 199309L
#include "common.h"
#include <time.h>

int main() 
{
    int fd = open(IN_FILE_NAME, O_RDONLY);
    struct stat st;
    fstat(fd, &st);
    size_t file_size = st.st_size;

    int sem_sender = sem_ctor(SEM_SENDER_KEY, 1);
    int sem_receiver = sem_ctor(SEM_RECEIVER_KEY, 0);

    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    shm_file_t* shm_file = shmat(shmid, NULL, 0);
    char* shm_data = (char*)shm_file + sizeof(shm_file_t); 

    shm_file->file_size = file_size;
    shm_file->data_size = 0;

    size_t total_sent = 0;
    size_t chunk_size = SHM_SIZE - sizeof(shm_file_t);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (total_sent < file_size) 
    {
        sem_wait(sem_sender); 
        
        size_t remaining = file_size - total_sent;
        size_t current_chunk = (remaining < chunk_size) ? remaining : chunk_size;
        
        size_t bytes_read = read(fd, shm_data, current_chunk);
        shm_file->data_size = current_chunk;
        
        total_sent += bytes_read;
        
        sem_sig(sem_receiver);
    }

    sem_wait(sem_sender);
    shm_file->data_size = 0;
    sem_sig(sem_receiver); 

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("time: %.3f seconds\n", time_taken);

    close(fd);
    shmdt(shm_file);
    shmctl(shmid, IPC_RMID, NULL);

    sem_dtor(sem_sender);
    sem_dtor(sem_receiver);

    return 0;
}