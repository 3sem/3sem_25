#include "common.h"

int main() 
{
    int dst_fd = open(OUT_FILE_NAME, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (dst_fd < 0) 
    {
        perror("file out");
        return 1;
    }

    sigset_t all_signals;
    sigfillset(&all_signals);
    if (sigprocmask(SIG_SETMASK, &all_signals, NULL) < 0) 
    {
        perror("sigprocmask");
        close(dst_fd);
        return 1;
    }

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd < 0) 
    {
        perror("shm_open");
        close(dst_fd);
        return 1;
    }

    void *shm_ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) 
    {
        perror("mmap");
        close(dst_fd);
        close(shm_fd);
        return 1;
    }

    struct shm_header* header = (struct shm_header*)shm_ptr;
    char* data_area = (char*)(header + 1);

    header->receiver_pid = getpid();

    sigset_t wait_usr1;
    sigemptyset(&wait_usr1);
    sigaddset(&wait_usr1, SIGUSR1);

    if (kill(header->sender_pid, SIGUSR2) < 0)
    {
        perror("sigusr2");
        return 1;
    }

    int sig;
    size_t total_received = 0;

    while (1) 
    {
        if (sigwait(&wait_usr1, &sig) != 0) 
        {
            perror("sigwait");
            break;
        }
        
        if (header->data_size == 0) 
            break;

        ssize_t bytes_written = write(dst_fd, data_area, header->data_size);
        if (bytes_written < 0) 
        {
            perror("file input error");
            break;
        }

        total_received += bytes_written;

        if (kill(header->sender_pid, SIGUSR2) < 0) 
        {
            perror("SIGUSR2");
            break;
        }
    }

    kill(header->sender_pid, SIGUSR2);

    munmap(shm_ptr, BUFFER_SIZE);
    close(shm_fd);
    close(dst_fd);
    
    return 0;
}