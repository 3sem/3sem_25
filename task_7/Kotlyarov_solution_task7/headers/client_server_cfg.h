#ifndef CLIENT_SERVER_CFG
#define CLIENT_SERVER_CFG

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#define CLIENT_SERVER_FIFO "Client_server_fifo"
#define ACKNOWLEDGE_CMD "ACK"
#define MAX_EVENTS 64

typedef struct {
    int tx_fd; 
    int rx_fd;
    char tx_path[128];
    char rx_path[128];
    int client_id;
    int registered;
} client_t;

#endif
