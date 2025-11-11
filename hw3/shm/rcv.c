#include "common.h"


int main() 
{
    int sem_sender = sem_ctor(SEM_SENDER_KEY, 1);
    int sem_receiver = sem_ctor(SEM_RECEIVER_KEY, 0);

    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666 | IPC_CREAT);
    shm_file_t* shm_file = shmat(shmid, NULL, 0);
    char* shm_data = (char*)shm_file + sizeof(shm_file_t);

    int fd = open(OUT_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    while (1) 
    {
        sem_wait(sem_receiver);

        if (!shm_file->data_size)
            break;
            
        write(fd, shm_data, shm_file->data_size); 

        sem_sig(sem_sender);
    }

    close(fd);
    shmdt(shm_file);

    return 0;
}