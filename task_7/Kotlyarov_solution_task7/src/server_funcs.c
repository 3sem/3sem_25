#include "server_funcs.h"

int server_should_exit = 0;
clients_t clients = { .amount = 0 };

void server_signal_handler(int sig) {
    DEBUG_PRINTF("\nReceived signal %d, shutting down server...\n", sig);
    server_should_exit = 1;
}

void setup_server_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = server_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);   // Ctrl+C
    sigaction(SIGTERM, &sa, NULL);  // kill command
    sigaction(SIGQUIT, &sa, NULL);  // Ctrl+ backslash
    
    signal(SIGPIPE, SIG_IGN);
}

void cleanup_server_resources(int cmd_fifo_fd, int epoll_fd, tx_threads_pool_t* pool) {
    DEBUG_PRINTF("Cleaning up server resources...\n");
    
    if (pool) {
        tx_pool_destroy(pool);
    }
    
    if (epoll_fd != -1) {
        close(epoll_fd);
    }
    
    if (cmd_fifo_fd != -1) {
        close(cmd_fifo_fd);
    }
    
    if (file_exists(CLIENT_SERVER_FIFO)) {
        remove_fifo_and_empty_dirs(CLIENT_SERVER_FIFO);
        DEBUG_PRINTF("Removed server FIFO: %s\n", CLIENT_SERVER_FIFO);
    }
    
    for (int i = 0; i < clients.amount; i++) {
        if (clients.clients[i].registered) {
            close_client_connection(&clients, i);
        }
    }
    
    DEBUG_PRINTF("Server cleanup completed\n");
}

void run_server_loop(tx_threads_pool_t* pool, int epoll_fd, int cmd_fifo_fd) {
    struct epoll_event events[MAX_EVENTS];
    
    DEBUG_PRINTF("Server loop started\n");
    
    while (!server_should_exit) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000); // 1 second timeout
        
        if (nfds == -1) {
            if (errno == EINTR) {
                if (server_should_exit) {
                    break;
                }
                continue;
            }
            perror("epoll_wait");
            continue;
        }
        
        if (nfds == 0) {
            if (server_should_exit) {
                break;
            }
            continue;
        }
        
        for (int i = 0; i < nfds && !server_should_exit; ++i) {
            uint32_t data_type = events[i].data.u32;
            
            if (data_type == FD_TYPE_CMD_FIFO) {
                DEBUG_PRINTF("Processing registration command\n");
                handle_registration_command(cmd_fifo_fd, &clients, epoll_fd);
            } else {
                int client_id = data_type;
                DEBUG_PRINTF("Processing GET command from client %d\n", client_id);
                handle_get_command(pool, client_id, &clients);
            }
        }
    }
    
    DEBUG_PRINTF("Server loop exiting...\n");
}

void close_client_connection(clients_t* clients, int client_id) {
    if (!clients || client_id < 0 || client_id >= clients->amount || 
        !clients->clients[client_id].registered) {
        return;
    }
    
    client_t* client = &clients->clients[client_id];
    
    if (client->tx_fd != -1) {
        close(client->tx_fd);
        client->tx_fd = -1;
    }
    
    if (client->rx_fd != -1) {
        close(client->rx_fd);
        client->rx_fd = -1;
    }
    
    if (strlen(client->tx_path) > 0) {
        remove_fifo_and_empty_dirs(client->tx_path);
    }
    
    if (strlen(client->rx_path) > 0) {
        remove_fifo_and_empty_dirs(client->rx_path);
    }
    
    DEBUG_PRINTF("Cleaned up client %d FIFOs: %s, %s\n", 
                 client_id, client->tx_path, client->rx_path);
    
    client->registered = 0;
    client->client_id = -1;
    client->tx_path[0] = '\0';
    client->rx_path[0] = '\0';
    
    clients->amount--;
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
    
    size_t total_read = 0;
    ssize_t n;
    
    while ((n = read(cmd_fifo_fd, buffer + total_read, sizeof(buffer) - 1 - total_read)) > 0) {
        total_read += (size_t)n;
        if (total_read >= sizeof(buffer) - 1) {
            break;
        }
    }
    
    if (total_read <= 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            DEBUG_PRINTF("ERROR: read from command FIFO failed\n");
            perror("read");
        }
        return -1;
    }

    buffer[total_read] = '\0';
    DEBUG_PRINTF("Received registration: %s\n", buffer);

    char* line = buffer;
    char* next_line;
    while (line && *line) {
        next_line = strchr(line, '\n');
        if (next_line) {
            *next_line = '\0';
            next_line++;
        }
        
        if (sscanf(line, "REGISTER %127s %127s", tx_path, rx_path) == 2) {
            DEBUG_PRINTF("Parsed TX: %s, RX: %s\n", tx_path, rx_path);
            
            client_t* new_client = create_client(clients, tx_path, rx_path);
            if (!new_client) {
                DEBUG_PRINTF("ERROR: Failed to create client\n");
                line = next_line;
                continue;
            }

            DEBUG_PRINTF("Client %d created, adding to epoll\n", new_client->client_id);
            
            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = new_client->rx_fd;
            event.data.u32 = new_client->client_id; 
            
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client->rx_fd, &event) == -1) {
                perror("epoll_ctl");
                close(new_client->tx_fd);
                close(new_client->rx_fd);
                new_client->registered = 0;
                line = next_line;
                continue;
            }
            
            DEBUG_PRINTF("Sending ACK to client\n");
            if (send_ack_to_client(new_client->tx_fd) != 0) {
                DEBUG_PRINTF("ERROR: Failed to send ACK\n");
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, new_client->rx_fd, NULL);
                close(new_client->tx_fd);
                close(new_client->rx_fd);
                new_client->registered = 0;
                line = next_line;
                continue;
            }
            
            ++(clients->amount);
            DEBUG_PRINTF("Client %d registered successfully. Total clients: %d\n", 
                       new_client->client_id, clients->amount);
        }
        
        line = next_line;
    }
    
    return 0;
}

int handle_get_command(tx_threads_pool_t* thread_pool, int client_id, clients_t* clients) {
    if (!clients || client_id < 0 || client_id >= clients->amount || 
        !clients->clients[client_id].registered) {
        DEBUG_PRINTF("ERROR: invalid client id %d\n", client_id);
        return -1;
    }
    
    client_t* client = &clients->clients[client_id];
    char buffer[256];
    char file_path[256];
    
    ssize_t n;
    while ((n = read(client->rx_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        
        char* line = buffer;
        char* next_line;
        while (line && *line) {
            next_line = strchr(line, '\n');
            if (next_line) {
                *next_line = '\0';
                next_line++;
            }
            
            if (sscanf(line, "GET %255s", file_path) == 1) {
                DEBUG_PRINTF("Submitting task for client %d, file: %s\n", client_id, file_path);
                
                if (tx_pool_submit_task(thread_pool, client_id, 
                                        client->tx_fd, file_path) != 0) {
                    DEBUG_PRINTF("ERROR: get failed: can't transmit file\n");
                } 
            } else {
                DEBUG_PRINTF("ERROR: invalid GET command: %s\n", line);
            }
            
            line = next_line;
        }
    }
    
    if (n == 0) {
        DEBUG_PRINTF("Client %d disconnected\n", client_id);
        close(client->tx_fd);
        close(client->rx_fd);
        client->registered = 0;
        clients->amount--;
    } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read from client rx_fd");
    }

    return 0;
}

client_t* create_client(clients_t* clients, const char* tx_path, const char* rx_path) {
    int client_id = find_free_client_slot(clients);
    if (client_id == -1) {
        return NULL;
    }

    client_t* client = &clients->clients[client_id];

    client->tx_fd = open(tx_path, O_WRONLY);
    if (client->tx_fd == -1) {
        perror("open client tx fifo");
        return NULL;
    }

    client->rx_fd = open(rx_path, O_RDONLY | O_NONBLOCK);
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

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients->clients[i].registered) {
            return i;
        }
    }

    DEBUG_PRINTF("ERROR: clients are full\n");
    return -1;
}

int send_ack_to_client(int client_tx_fd) {
    if (write(client_tx_fd, ACKNOWLEDGE_CMD, strlen(ACKNOWLEDGE_CMD)) == -1) {
        perror("write ACK");
        return -1;
    } 

    return 0;
}

int send_file_to_client(int client_tx_fd, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t) ftell(file);
    fseek(file, 0, SEEK_SET);

    char size_info[64];
    snprintf(size_info, sizeof(size_info), "SIZE:%ld\n", file_size);
    if (write(client_tx_fd, size_info, strlen(size_info)) <= 0) {
        perror("write size info");
        fclose(file);
        return -1;
    }

    DEBUG_PRINTF("Sending file of size: %ld\n", file_size);
    
    char buffer[4096];
    size_t total_bytes_sent = 0;
    
    while (total_bytes_sent < file_size) {
        size_t bytes_to_read = sizeof(buffer);
        if (file_size - total_bytes_sent < bytes_to_read) {
            bytes_to_read = file_size - total_bytes_sent;
        }

        size_t bytes_read = fread(buffer, 1, bytes_to_read, file);
        
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
