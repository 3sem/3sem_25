#include "common.h"
#include <time.h>

int main() 
{
    int src_fd = open(IN_FILE_NAME, O_CREAT | O_RDONLY);
    if (src_fd < 0) 
    {
        perror("open in");
        return 1;
    }
    
    sigset_t all_signals;
    sigfillset(&all_signals);
    if (sigprocmask(SIG_SETMASK, &all_signals, NULL) < 0) 
    {
        perror("sigprocmask");
        close(src_fd);
        return 1;
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) 
    {
        perror("shm_open");
        close(src_fd);
        return 1;
    }

    if (ftruncate(shm_fd, BUFFER_SIZE) < 0) 
    {
        perror("ftruncate");
        close(src_fd);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    void* shm_ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) 
    {
        perror("mmap");
        close(src_fd);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return 1;
    }

    struct shm_header* header = (struct shm_header*)shm_ptr;
    char* data_area = (char*)(header + 1);

    header->sender_pid = getpid();
    header->receiver_pid = 0;

    sigset_t wait_usr2;
    sigemptyset(&wait_usr2);
    sigaddset(&wait_usr2, SIGUSR2);

    int sig;
    if (sigwait(&wait_usr2, &sig) != 0) 
    {
        perror("sigwait");
        return 1;
    }

    ssize_t bytes_read;
    size_t total_sent = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (1) 
    {
        bytes_read = read(src_fd, data_area, MAX_DATA_SIZE);
        if (bytes_read < 0) 
        {
            perror("read error");
            break;
        }

        header->data_size = bytes_read;
        total_sent += bytes_read;

        if (kill(header->receiver_pid, SIGUSR1) < 0) 
        {
            perror("SIGUSR1");
            break;
        }

        if (bytes_read == 0) 
            break;

        if (sigwait(&wait_usr2, &sig) != 0) 
        {
            perror("sigwait");
            break;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("time: %.3f seconds\n", time_taken);

    munmap(shm_ptr, BUFFER_SIZE);
    close(shm_fd);
    close(src_fd);
    
    //sleep(1);
    
    shm_unlink(SHM_NAME);
    
    return 0;
}