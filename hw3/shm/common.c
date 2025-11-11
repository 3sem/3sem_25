#include "common.h"

int sem_ctor(int key, int initial_value) 
{
    int semid = semget(key, 1, 0666 | IPC_CREAT);
    
    semctl(semid, 0, SETVAL, initial_value);
    
    return semid;
}

void sem_wait(int semid) 
{
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}

void sem_sig(int semid) 
{
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}

void sem_dtor(int semid) 
{
    semctl(semid, 0, IPC_RMID);
}