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

void ready_handler(int sig, siginfo_t *info, void* context);
void complete_handler(int sig, siginfo_t *info, void* context);
void sender_process(struct SignalInfo* signal_info);
void receiver_process(struct SignalInfo* signal_info);

void ready_handler(int sig, siginfo_t *info, void* context) {
	struct SignalInfo* signal_info = (struct SignalInfo*)info->si_value.sival_ptr;
	signal_info->ready_for_transfer = 1;
    	return;
}

void complete_handler(int sig, siginfo_t *info, void* context) {
    	struct SignalInfo* signal_info = (struct SignalInfo*)info->si_value.sival_ptr;
        signal_info->transfer_complete = 1;
    	return;
}

void sender_process(struct SignalInfo* signal_info) {
    printf("SENDER: run process\n");
    
    struct sigaction sa;
    sa.sa_sigaction = ready_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
    
    sa.sa_sigaction = complete_handler;
    sigaction(SIGUSR2, &sa, NULL);

    while (!signal_info->ready_for_transfer) {};

    printf("SENDER: Receiver is ready, opening file: %s\n", signal_info->filename);
    
    int src_file = open(signal_info->filename, O_RDONLY);
    if (src_file == -1) {
        perror("open source file");
        unlink(FIFO_FILE);
        exit(1);
    }

    int fifo_fd = open(FIFO_FILE, O_WRONLY);
    if (fifo_fd == -1) {
        perror("open FIFO for writing");
        close(src_file);
        unlink(FIFO_FILE);
        exit(1);
    }

    printf("SENDER: Start sending data\n");
    
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long total_bytes = 0;
    
    while ((bytes_read = (size_t)read(src_file, buffer, (size_t)BUFFER_SIZE)) > 0) {
        ssize_t bytes_written = write(fifo_fd, buffer, (size_t)bytes_read);
        if (bytes_written > 0) {
            total_bytes += bytes_written;
        }
    }

    printf("SENDER: sended %ld bytes\n", total_bytes);
    
    close(fifo_fd);
    close(src_file);

    printf("SENDER: waiting finish\n");
    
    while (!signal_info->transfer_complete) {};

    unlink(FIFO_FILE);
    printf("SENDER: Complete!\n");
}

void receiver_process(struct SignalInfo* signal_info) {
    printf("RECEIVER: run process\n");
    
    struct sigaction sa;
    sa.sa_sigaction = complete_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &sa, NULL);

    printf("RECEIVER: send ready signal\n");
    union sigval value = {.sival_ptr = signal_info};
    sigqueue(signal_info->sender_pid, SIGUSR1, value);

    printf("RECEIVER: opening FIFO for reading\n");
    int fifo_fd = open(FIFO_FILE, O_RDONLY);
    if (fifo_fd == -1) {
        perror("open FIFO for reading");
        exit(1);
    }

    int dst_file = open(RECEIVED_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (dst_file == -1) {
        perror("open dest file");
        close(fifo_fd);
        exit(1);
    }

    printf("RECEIVER: start receiving data\n");
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    long total_bytes = 0;
    
    while ((bytes_read = read(fifo_fd, buffer, (size_t)BUFFER_SIZE)) > 0) {
        ssize_t bytes_written = write(dst_file, buffer, (size_t)bytes_read);
        total_bytes += bytes_written;
    }

    printf("RECEIVER: received %ld bytes\n", total_bytes);
    
    close(fifo_fd);
    close(dst_file);

    printf("RECEIVER: give signal for success\n");
    sigqueue(signal_info->sender_pid, SIGUSR2, value);
    
    printf("RECEIVER: save data as new file %s\n", RECEIVED_FILE);
    return;
}
