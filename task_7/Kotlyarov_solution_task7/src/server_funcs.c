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
    clients_t clients = { 
        .amount = 0,
        .cmd_buffer = { .pos = 0 }
    };
    
    DEBUG_PRINTF("Server loop started\n");
    
    while (!server_should_exit) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        
        if (nfds == -1) {
            if (errno == EINTR) {
                if (server_should_exit) break;
                continue;
            }
            perror("epoll_wait");
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

int handle_registration_command(int cmd_fifo_fd, clients_t* clients, int epoll_fd) {
    if (!clients) {
        DEBUG_PRINTF("ERROR: registration failed: null ptr\n");
        return -1;
    }

    char read_buffer[1024];
    char tx_path[256];
    char rx_path[256];
    
    DEBUG_PRINTF("Reading registration commands...\n");
    
    ssize_t n = read(cmd_fifo_fd, read_buffer, sizeof(read_buffer) - 1);
    if (n <= 0) {
        if (n == 0) {
            DEBUG_PRINTF("EOF on command FIFO\n");
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read from command FIFO");
        }
        return -1;
    }

    read_buffer[n] = '\0';
    
    fifo_buffer_t* buf = &clients->cmd_buffer;
    size_t available = sizeof(buf->buffer) - buf->pos - 1;
    
    if ((size_t)n > available) {
        DEBUG_PRINTF("WARNING: Command buffer overflow, resetting\n");
        buf->pos = 0;
        available = sizeof(buf->buffer) - 1;
    }
    
    size_t to_copy = (size_t)n < available ? (size_t)n : available;
    memcpy(buf->buffer + buf->pos, read_buffer, to_copy);
    buf->pos += to_copy;
    buf->buffer[buf->pos] = '\0';
    
    DEBUG_PRINTF("Command buffer: [%s] (%zu bytes)\n", buf->buffer, buf->pos);
    
    char* line_start = buf->buffer;
    int commands_processed = 0;
    
    while (line_start < buf->buffer + buf->pos) {
        char* line_end = strchr(line_start, '\n');
        if (!line_end) {
            break;
        }
        
        *line_end = '\0';
        
        if (sscanf(line_start, "REGISTER %255s %255s", tx_path, rx_path) == 2) {
            DEBUG_PRINTF("Parsed command: TX=%s, RX=%s\n", tx_path, rx_path);
            
            client_t* new_client = create_client(clients, tx_path, rx_path);
            if (new_client) {
                DEBUG_PRINTF("Client %d created\n", new_client->client_id);
                
                struct epoll_event event;
                event.events = EPOLLIN;
                event.data.fd = new_client->rx_fd;
                event.data.u32 = new_client->client_id;
                
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client->rx_fd, &event) == -1) {
                    perror("epoll_ctl");
                    close_client_connection(clients, new_client->client_id);
                    line_start = line_end + 1;
                    continue;
                }
                
                if (send_ack_to_client(new_client->tx_fd) != 0) {
                    DEBUG_PRINTF("ERROR: Failed to send ACK\n");
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, new_client->rx_fd, NULL);
                    close_client_connection(clients, new_client->client_id);
                    line_start = line_end + 1;
                    continue;
                }
                
                clients->amount++;
                DEBUG_PRINTF("Client %d registered. Total: %d\n", 
                           new_client->client_id, clients->amount);
                commands_processed++;
            } else {
                DEBUG_PRINTF("ERROR: Failed to create client\n");
            }
        } else {
            DEBUG_PRINTF("ERROR: Invalid registration command: %s\n", line_start);
        }
        
        line_start = line_end + 1;
    }
    
    size_t remaining = buf->buffer + buf->pos - line_start;
    if (remaining > 0) {
        memmove(buf->buffer, line_start, remaining);
        buf->pos = remaining;
        buf->buffer[buf->pos] = '\0';
        DEBUG_PRINTF("Remaining in buffer: [%s] (%zu bytes)\n", buf->buffer, buf->pos);
    } else {
        buf->pos = 0;
    }
    
    DEBUG_PRINTF("Processed %d registration commands\n", commands_processed);
    return commands_processed > 0 ? 0 : -1;
}

int handle_get_command(tx_threads_pool_t* thread_pool, int client_id, clients_t* clients) {
    if (!clients || client_id < 0 || client_id >= clients->amount || 
        !clients->clients[client_id].registered) {
        DEBUG_PRINTF("ERROR: invalid client id %d (total: %d)\n", client_id, clients->amount);
        return -1;
    }
    
    client_t* client = &clients->clients[client_id];
    char buffer[1024];
    char file_path[256];
    
    DEBUG_PRINTF("Reading from client %d (fd: %d)...\n", client_id, client->rx_fd);
    ssize_t n = read(client->rx_fd, buffer, sizeof(buffer) - 1);
    
    if (n > 0) {
        buffer[n] = '\0';
        DEBUG_PRINTF("Client %d sent %zd bytes: '%s'\n", client_id, n, buffer);
        
        char* line_end = strchr(buffer, '\n');
        if (line_end) *line_end = '\0';
        
        if (sscanf(buffer, "GET %255s", file_path) == 1) {
            DEBUG_PRINTF("✅ Client %d requested file: %s\n", client_id, file_path);
            
            // Проверим существование файла
            if (file_exists(file_path)) {
                DEBUG_PRINTF("❌ File not found: %s\n", file_path);
                return -1;
            }
            
            struct stat st;
            if (stat(file_path, &st) == 0) {
                DEBUG_PRINTF("📁 File size: %ld bytes\n", st.st_size);
            }
            
            if (tx_pool_submit_task(thread_pool, client_id, client->tx_fd, file_path) != 0) {
                DEBUG_PRINTF("❌ Failed to submit task for client %d\n", client_id);
                return -1;
            }
            DEBUG_PRINTF("✅ Task submitted for client %d\n", client_id);
        } else {
            DEBUG_PRINTF("❌ Invalid command from client %d: '%s'\n", client_id, buffer);
            return -1;
        }
    } else if (n == 0) {
        DEBUG_PRINTF("📞 No data from client %d (EOF)\n", client_id);
        return 0;
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            DEBUG_PRINTF("❌ Read error from client %d: ", client_id);
            perror("");
            close_client_connection(clients, client_id);
        }
        return -1;
    }

    return 0;
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
        DEBUG_PRINTF("❌ Cannot open file: %s - ", filename);
        perror("");
        return -1;
    }

    // Получаем размер файла для отладки
    struct stat st;
    if (stat(filename, &st) == 0) {
        DEBUG_PRINTF("📤 Sending file: %s (%ld bytes) to fd %d\n", 
                     filename, st.st_size, client_tx_fd);
    }

    char buffer[4096];
    size_t total_bytes_sent = 0;
    
    while (1) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        
        if (bytes_read == 0) {
            if (ferror(file)) {
                DEBUG_PRINTF("❌ File read error: %s\n", filename);
                fclose(file);
                return -1;
            }
            break; // EOF
        }
        
        // ✅ Атомарная отправка всего куска
        size_t bytes_sent = 0;
        while (bytes_sent < bytes_read) {
            ssize_t result = write(client_tx_fd, buffer + bytes_sent, bytes_read - bytes_sent);
            
            if (result == -1) {
                if (errno == EINTR) continue;
                DEBUG_PRINTF("❌ Write error to client fd %d: ", client_tx_fd);
                perror("");
                fclose(file);
                return -1;
            }
            
            if (result == 0) {
                DEBUG_PRINTF("❌ FIFO closed by client (fd: %d)\n", client_tx_fd);
                fclose(file);
                return -1;
            }
            
            bytes_sent += result;
            total_bytes_sent += result;
        }
    }

    fclose(file);
    DEBUG_PRINTF("✅ File %s sent successfully (%zu bytes)\n", filename, total_bytes_sent);
    return 0;
}
