#include "server_funcs.h"

void run_server_loop(int epoll_fd, tx_threads_pool_t* thread_pool) {
    struct epoll_event events[MAX_EVENTS];
    clients_t clients = { .amount = 0};
    
    DEBUG_PRINTF("Server loop started\n");
    
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            continue;
        }
        
        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            uint32_t data = events[i].data.u32;
            
            if (data == FD_TYPE_CMD_FIFO) {
                DEBUG_PRINTF("Processing registration command\n");
                if(handle_registration_command(fd, &clients, epoll_fd) != 0)
                    return;
            } else {
                int client_id = data;
                DEBUG_PRINTF("Processing GET command from client %d\n", client_id);
                handle_get_command(thread_pool, client_id, &clients);
            }
        }
    }
}

int handle_registration_command(int cmd_fifo_fd, clients_t* clients, int epoll_fd) {
    if (!clients) {
        DEBUG_PRINTF("ERROR: registration failed: null ptr\n");
        return -1;
    }

    char buffer[512];
    char tx_path[256];
    char rx_path[256];
    
    DEBUG_PRINTF("Reading registration command...\n");
    ssize_t n = read(cmd_fifo_fd, buffer, sizeof(buffer) - 1);

    if (n <= 0) {
        DEBUG_PRINTF("ERROR: n <=0\n");
        if (n == 0) {
            DEBUG_PRINTF("EOF on command FIFO\n");
        } else {
            perror("read from command FIFO");
        }
        return -1;
    }

    buffer[n] = '\0';
    DEBUG_PRINTF("Received registration: %s\n", buffer);

    if (sscanf(buffer, "REGISTER %127s %127s", tx_path, rx_path) != 2) {
        DEBUG_PRINTF("ERROR: Failed to parse registration command\n");
        return -1;
    }

    DEBUG_PRINTF("Parsed TX: %s, RX: %s\n", tx_path, rx_path);
    
    client_t* new_client = create_client(clients, tx_path, rx_path);
    if (!new_client) {
        DEBUG_PRINTF("ERROR: Failed to create client\n");
        return -1;
    }

    DEBUG_PRINTF("Client %d created, adding to epoll\n", new_client->client_id);
    
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = new_client->rx_fd;
    event.data.u32 = new_client->client_id;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client->rx_fd, &event) == -1) {
        perror("epoll_ctl");
        return -1;
    }
    
    DEBUG_PRINTF("Sending ACK to client\n");
    if (send_ack_to_client(new_client->tx_fd) != 0) {
        DEBUG_PRINTF("ERROR: Failed to send ACK\n");
        return -1;
    }
    
    ++(clients->amount);
    DEBUG_PRINTF("Client %d registered successfully. Total clients: %d\n", 
               new_client->client_id, clients->amount);
    
    return 0;
}

int send_ack_to_client(int client_tx_fd) {
    if (write(client_tx_fd, ACKNOWLEDGE_CMD, strlen(ACKNOWLEDGE_CMD)) == -1) {
        perror("write ACK");
        return -1;
    } 

    return 0;
}

int handle_get_command(tx_threads_pool_t* thread_pool, int client_id, clients_t* clients) {
    if (!clients) {
        DEBUG_PRINTF("ERROR: null ptr\n");
        return -1;
    }
    
    char buffer[256];
    char file_path[256];
    ssize_t n = read(clients->clients[client_id].rx_fd, buffer, sizeof(buffer) - 1);

    if (n > 0) {
        buffer[n] = '\0';

        if (sscanf(buffer, "GET %255s", file_path) == 1) {
            client_t* client = clients->clients + client_id;
    
            if (tx_pool_submit_task(thread_pool, client_id, 
                                    client->tx_fd, file_path) != 0) {
                DEBUG_PRINTF("ERROR: get failed: can't transmit file\n");
                return -1;
            }
        } else {
            DEBUG_PRINTF("ERROR: get failed: can't recieve filepath\n");
            return -1;
        }
    }

    return 0;
}

client_t* create_client(clients_t* clients, const char* tx_path, const char* rx_path) {
    int client_id = find_free_client_slot(clients);
    if (client_id == -1) {
        return NULL;
    }

    client_t* client = clients->clients + client_id;

    client->tx_fd = open(tx_path, O_WRONLY);
    if (client->tx_fd == -1) {
        perror("open client tx fifo");
        return NULL;
    }

    client->rx_fd = open(rx_path, O_RDWR | O_NONBLOCK);
    if (client->rx_fd == -1) {
        perror("open client rx fifo");
        close(client->tx_fd);
        return NULL;
    }

    strncpy(client->tx_path, tx_path, sizeof(client->tx_path) - 1);
    strncpy(client->rx_path, rx_path, sizeof(client->rx_path) - 1);
    client->client_id = client_id;
    client->registered = 1;

    return client;
}

int find_free_client_slot(clients_t* clients) {
    if (!clients) {
        DEBUG_PRINTF("ERROR: null ptr\n");
        return -1;
    }

    int amount = clients->amount;
    for (int i = 0; i < amount; ++i) {
        if (!clients->clients[i].registered) {
            return i;
        }
    }

    DEBUG_PRINTF("Warning: clients are full\n");
    return -1;
}

int send_file_to_client(int client_tx_fd, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        return -1;
    }

    char buffer[4096];
    size_t total_bytes_sent = 0;
    
    while (1) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        
        if (bytes_read == 0) {
            if (ferror(file)) {
                perror("fread");
                fclose(file);
                return -1;
            }
            break; 
        }
        
        size_t bytes_sent = 0;
        while (bytes_sent < bytes_read) {
            ssize_t result = write(client_tx_fd, buffer + bytes_sent, bytes_read - bytes_sent);
            
            if (result == -1) {
                if (errno == EINTR) {
                    continue;
                }
                perror("write");
                fclose(file);
                return -1;
            }
            
            if (result == 0) {
                DEBUG_PRINTF("FIFO closed by client\n");
                fclose(file);
                return -1;
            }
            
            bytes_sent += result;
            total_bytes_sent += result;
        }
    }

    if (fclose(file) == EOF) {
        perror("fclose");
        return -1;
    }
    
    return 0;
}
