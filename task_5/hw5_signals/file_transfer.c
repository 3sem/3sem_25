#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_FILENAME 256
#define CHUNK_SIZE 4

int output_file;
char buffer_a[4] = {};
int current_chunk_size = 4; 

void handler(int sig, siginfo_t *info, void *ucontext) {
    if (sig == SIGRTMIN + 1) {
        memcpy(buffer_a, &(info->si_value.sival_int), 4);
        current_chunk_size = 4;
    } else if (sig == SIGRTMIN) {
        memcpy(buffer_a, &(info->si_value.sival_int), 4);
        current_chunk_size = info->si_value.sival_int >> 24;
    }
    
    write(output_file, buffer_a, current_chunk_size);
}

int main(int argc, char *argv[]) {
    char *input_filename = argv[1];
    char output_filename[MAX_FILENAME];
    snprintf(output_filename, sizeof(output_filename), "copy_%s", input_filename);

    int input_fd = open(input_filename, O_RDONLY);
    output_file = open(output_filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    struct sigaction sa = {.sa_sigaction = handler, .sa_flags = SA_SIGINFO};
    sigaction(SIGRTMIN, &sa, NULL);   
    sigaction(SIGRTMIN + 1, &sa, NULL); 

    pid_t pid = fork();
    
    if (pid == 0) {
        while (1) pause();
    } else {
        usleep(100000);
        
        char buffer[4];
        int bytes_read;

        while ((bytes_read = read(input_fd, buffer, 4)) > 0) {
            int result = 0;
            memcpy(&result, buffer, bytes_read);
            
            if (bytes_read < 4) {
                int packed_data = (bytes_read << 24) | (result & 0x00FFFFFF);
                union sigval sv = {.sival_int = packed_data};
                sigqueue(pid, SIGRTMIN, sv);
            } else {
                union sigval sv = {.sival_int = result};
                sigqueue(pid, SIGRTMIN + 1, sv);
            }
        }

        sleep(1);
        kill(pid, SIGTERM);
        
        close(input_fd);
        close(output_file);
    }
    
    return 0;
}