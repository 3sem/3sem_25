#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "Signal.hpp"

void receiver_process(struct SignalInfo* signal_info);
void sender_process(struct SignalInfo* signal_info);

int main(int argc, char* argv[]) {
    struct SignalInfo signal_info = {};
    char filename[] = SOURCE_FILE;

    if (argc != 2)
        signal_info.filename = filename;
    else 
	signal_info.filename = argv[1];

    struct timespec start, end;
    double transmission_time = 0;

    if (mkfifo(FIFO_FILE, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(1);
    }

    pid_t pid = fork();
    
    if (pid == 0) {
	signal_info.sender_pid = getppid();
        receiver_process(&signal_info);
	return 0;
    } else {
	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
		perror("clock_gettime");
		return 1;
	}

        sender_process(&signal_info);
        wait(NULL);

	if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
		perror("clock_gettime");
		return 1;
	}
    }	

    transmission_time = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / (double)BILLION;
    printf("=====TIME=====\nOverall transmission time: %lf seconds\n", transmission_time);
    
    return 0;
}
