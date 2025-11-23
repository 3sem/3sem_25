#include "client_funcs.h"

static char global_tx_path[256] = "";
static char global_rx_path[256] = "";

int register_client(client_t* client_info, int cl_s_fifo_fd) {
    DEBUG_PRINTF("REGISTRATION\n");
    if (!client_info) {
        DEBUG_PRINTF("ERROR: null client info\n");
        return -1;
    }

    char reg_cmd[512]; 
    char response[16];
    
    int written = snprintf(reg_cmd, sizeof(reg_cmd), "REGISTER %s %s", 
                          client_info->tx_path, client_info->rx_path);
    
    if (written < 0 || (size_t)written >= sizeof(reg_cmd)) {
        DEBUG_PRINTF("ERROR: registration command too long\n");
        return -1;
    }

    DEBUG_PRINTF("Registration command: %s\n", reg_cmd);
    
    int temp_tx_fd = open(client_info->tx_path, O_RDONLY | O_NONBLOCK);
    if (temp_tx_fd == -1) {
        perror("open TX FIFO for registration");
        return -1;
    }

    if (write(cl_s_fifo_fd, reg_cmd, strlen(reg_cmd)) <= 0) {
        perror("write registration");
        close(temp_tx_fd);
        return -1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(temp_tx_fd);
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = temp_tx_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, temp_tx_fd, &event) == -1) {
        perror("epoll_ctl");
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    }

    struct epoll_event events[1];
    int timeout_ms = 5000; // 5 s
    
    int nfds = epoll_wait(epoll_fd, events, 1, timeout_ms);
    
    if (nfds == -1) {
        perror("epoll_wait");
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    } else if (nfds == 0) {
        DEBUG_PRINTF("ERROR: Registration timeout\n");
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    }
    
    ssize_t n = read(temp_tx_fd, response, sizeof(response) - 1);
    if (n <= 0) {
        perror("read ACK");
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    }
    response[n] = '\0';

    DEBUG_PRINTF("Received response: %s\n", response);
    
    if (strncmp(response, ACKNOWLEDGE_CMD, strlen(ACKNOWLEDGE_CMD)) != 0) {
        DEBUG_PRINTF("ERROR: wrong acknowledge: %s\n", response);
        close(temp_tx_fd);
        close(epoll_fd);
        return -1;
    }

    close(temp_tx_fd);
    close(epoll_fd);
    
    DEBUG_PRINTF("Registration successful\n");
    return 0;
}

int setup_client_fifos(const char* tx_path, const char* rx_path) {
    DEBUG_PRINTF("Setting up client FIFOs...\n");

    strncpy(global_tx_path, tx_path, sizeof(global_tx_path) - 1);
    strncpy(global_rx_path, rx_path, sizeof(global_rx_path) - 1);

    if (create_fifo(tx_path) != 0) {
        DEBUG_PRINTF("ERROR: Failed to create TX FIFO: %s\n", tx_path);
        return -1;
    }

    if (create_fifo(rx_path) != 0) {
        DEBUG_PRINTF("ERROR: Failed to create RX FIFO: %s\n", rx_path);
        remove(tx_path);
        return -1;
    }

    return 0;
}

void cleanup_client_fifos(const char* tx_path, const char* rx_path) {
    DEBUG_PRINTF("Cleaning up client FIFOs.\n");

    remove_fifo_and_empty_dirs(tx_path);
    remove_fifo_and_empty_dirs(rx_path);
    global_tx_path[0] = '\0';
    global_rx_path[0] = '\0';
}

int client_recieve_and_print(int tx_fd) {
    if (tx_fd < 0) {
        DEBUG_PRINTF("ERROR: invalid file descriptor\n");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t total_bytes_read = 0;
    ssize_t bytes_read;
    
    while (1) {
        bytes_read = read(tx_fd, buffer, sizeof(buffer));
        
        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("read from FIFO");
            return -1;
        }
        
        if (bytes_read == 0) {
            break;
        }
        
        total_bytes_read += bytes_read;
        
        ssize_t bytes_written = 0;
        while (bytes_written < bytes_read) {
            ssize_t result = write(STDOUT_FILENO, buffer + bytes_written, bytes_read - bytes_written);
            
            if (result == -1) {
                if (errno == EINTR) {
                    continue; 
                }
                perror("write to stdout");
                return -1;
            }
            
            if (result == 0) {
                DEBUG_PRINTF("ERROR: stdout closed unexpectedly\n");
                return -1;
            }
            
            bytes_written += result;
        }
        
        if (fflush(stdout) == EOF) {
            perror("fflush");
            return -1;
        }
    }
    
    return 0;
}

int send_get_command(client_t* client_info, const char* filename) {
    if (!client_info || !filename) {
        DEBUG_PRINTF("ERROR: invalid arguments\n");
        return -1;
    }

    char command[512];
    snprintf(command, sizeof(command), "GET %s", filename);

    DEBUG_PRINTF("Sending command: %s\n", command);

    if (write(client_info->rx_fd, command, strlen(command)) <= 0) {
        perror("write GET command");
        return -1;
    }

    return 0;
}

void signal_handler(int sig) {
    DEBUG_PRINTF("\nReceived signal %d, cleaning up.\n", sig);

    if (strlen(global_tx_path) > 0 && strlen(global_rx_path) > 0) {
        cleanup_client_fifos(global_tx_path, global_rx_path);
    }

    exit(0);
}
