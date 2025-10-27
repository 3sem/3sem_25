#include "common.h"
#include "shm_transfer.h"

void shm_copy_file (const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file) 
    {
        perror("Fopen failed");
        return;
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    key_t key = ftok(".", 'S');
    if (key == -1) 
    {
        perror("ftok failed");
        fclose(file);
        return;
    }

    int shmid = shmget(key, sizeof(shared_mem_t), IPC_CREAT | 0666);
    if (shmid == -1) 
    {
        perror("shmget failed");
        fclose(file);
        return;
    }

    shared_mem_t *shm = (shared_mem_t*)shmat(shmid, NULL, 0);
    if (shm == (void*)-1) 
    {
        perror("shmat failed");
        fclose(file);
        shmctl(shmid, IPC_RMID, NULL);
        return;
    }

    shm->bytes_used = 0;
    shm->transfer_complete = 0;

    sem_t *sem_ready     = sem_open(SEM_READY_NAME, O_CREAT, 0666, 0);  
    sem_t *sem_processed = sem_open(SEM_DONE_NAME,  O_CREAT, 0666, 0);    
    
    if (sem_ready == SEM_FAILED || sem_processed == SEM_FAILED) 
    {
        perror("Sem_open failed");
        fclose(file);
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("Fork failed");
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(sem_ready);
        sem_close(sem_processed);
        sem_unlink(SEM_READY_NAME);
        sem_unlink(SEM_DONE_NAME);
        fclose(file);
        return;
    }
    else if (pid == 0) 
    {
        char output_filename[MAX_FILENAME];
        snprintf(output_filename, MAX_FILENAME, "copy_%s", filename);
        
        FILE *output_file = fopen(output_filename, "wb");
        if (!output_file) 
        {
            perror("Fopen failed");
            shmdt(shm);
            exit(1);
        }

        while (1) 
        {
            sem_wait(sem_ready);
            
            if (shm->transfer_complete) 
                break;
            
            if (shm->bytes_used > 0) 
                fwrite(shm->data, 1, shm->bytes_used, output_file);
            
            sem_post(sem_processed);
        }
        
        fclose(output_file);
        shmdt(shm);
        sem_close(sem_ready);
        sem_close(sem_processed);
        exit(0);
    } else {
        printf("---------------------------------------\n");
        printf("Starting transfering...\n");
        size_t total_sent = 0;
        size_t chunks_total = (file_size + BUF_SIZE - 1) / BUF_SIZE;

        struct timeval start_time;
        start_timing(&start_time);

        for (size_t chunk_num = 0; chunk_num < chunks_total; chunk_num++) {
            size_t bytes_read = fread(shm->data, 1, BUF_SIZE, file);
            shm->bytes_used = bytes_read;
            total_sent += bytes_read;

            sem_post(sem_ready);
            sem_wait(sem_processed);
        }

        printf("Transfering completed (%ld bytes).\n", total_sent);
        shm->transfer_complete = 1;
        sem_post(sem_ready); 
        wait(NULL);

        double copy_time = stop_timing(&start_time);
        printf("Total time: %.2f seconds\n", copy_time);
        printf("---------------------------------------\n");
        
        fclose(file);
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(sem_ready);
        sem_close(sem_processed);
        sem_unlink(SEM_READY_NAME);
        sem_unlink(SEM_DONE_NAME);
    }
}