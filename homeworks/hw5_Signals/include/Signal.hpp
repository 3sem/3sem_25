#pragma once

#define SOURCE_FILE "file.txt"
#define RECEIVED_FILE "received_file.txt"
#define FIFO_FILE "/tmp/file_transfer_fifo"
#define BUFFER_SIZE 4096
#define BILLION 1e9

struct SignalInfo {
        pid_t sender_pid;
        char* filename;
        int ready_for_transfer;
        int transfer_complete;
};
