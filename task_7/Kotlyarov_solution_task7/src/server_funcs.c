#include "server_funcs.h"

tx_threads_pool_t* global_pool = NULL;
int server_should_exit = 0;
clients_t* global_clients = NULL;
int global_epoll_fd = -1;
int global_cmd_fifo_fd = -1;

bool initialize_server_fifo(void) {
    if (create_fifo(CLIENT_SERVER_FIFO) != 0) {
        return false;
    }
    return true;
}

tx_threads_pool_t* initialize_thread_pool(void) {
    tx_threads_pool_t* pool = tx_pool_init(MIN_THREADS);
    if (!pool) {
        DEBUG_PRINTF("ERROR: Failed to initialize thread pool\n");
        return NULL;
    }
    global_pool = pool;
    return pool;
}

int open_command_fifo(void) {
    int cl_s_fifo_fd = open(CLIENT_SERVER_FIFO, O_RDWR | O_NONBLOCK);
    if (cl_s_fifo_fd == -1) {
        perror("open CLIENT_SERVER_FIFO");
        return -1;
    }
    global_cmd_fifo_fd = cl_s_fifo_fd;
    return cl_s_fifo_fd;
}

int setup_epoll(int cl_s_fifo_fd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }

    global_epoll_fd = epoll_fd;

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = cl_s_fifo_fd;
    event.data.u32 = FD_TYPE_CMD_FIFO;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cl_s_fifo_fd, &event) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        return -1;
    }

    return epoll_fd;
}

void server_signal_handler(int sig) {
    DEBUG_PRINTF("Received signal %d, shutting down server\n", sig);
    //DUMP_SERVER_STATE();
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

void run_server_loop(tx_threads_pool_t* pool, int epoll_fd, int cmd_fifo_fd) {
    struct epoll_event events[MAX_EVENTS];
    clients_t* clients = (clients_t*)calloc(1, sizeof(clients_t));
    if (!clients) {
        perror("calloc for clients");
        return;
    }

    clients->amount = 0;
    global_clients = clients;

    DEBUG_PRINTF("Server loop started\n");

    while (!server_should_exit) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

        if (nfds == -1) {
            if (errno == EINTR) {
                if (server_should_exit) break;
                continue;
            }
            perror("epoll_wait");
            server_should_exit = 1;
            continue;
        }

        for (int i = 0; i < nfds && !server_should_exit; ++i) {
            uint32_t data_type = events[i].data.u32;

            if (data_type == FD_TYPE_CMD_FIFO) {
                if (handle_registration_command(cmd_fifo_fd, clients, epoll_fd) != 0) {
                    server_should_exit = 1;
                }
            } else {
                int client_id = data_type;
                if (handle_get_command(pool, client_id, clients, epoll_fd) != 0) {
                    server_should_exit = 1;
                }
            }
        }
    }

    DEBUG_PRINTF("Server loop exiting\n");

    if (global_clients == clients) {
        global_clients = NULL;
    }
    free(clients);
}

int process_registration_buffer(fifo_buffer_t* buf, clients_t* clients, int epoll_fd) {
    int commands_processed = 0;
    char* current_pos = buf->buffer;
    char* last_complete = buf->buffer;

    while (current_pos < buf->buffer + buf->pos) {
        char* line_end = strchr(current_pos, '\n');
        if (!line_end) {
            break;
        }

        *line_end = '\0';

        char tx_path[256], rx_path[256];
        if (sscanf(current_pos, "REGISTER %255s %255s", tx_path, rx_path) == 2) {
            if (register_new_client(clients, tx_path, rx_path, epoll_fd) != 0) {
                DEBUG_PRINTF("ERROR: Failed to register client\n");
            } else {
                commands_processed++;
            }
        } else {
            DEBUG_PRINTF("ERROR: Invalid registration command: %s\n", current_pos);
        }

        current_pos = line_end + 1;
        last_complete = current_pos;
    }

    size_t remaining = buf->buffer + buf->pos - last_complete;
    if (remaining > 0) {
        memmove(buf->buffer, last_complete, remaining);
        buf->pos = remaining;
    } else {
        buf->pos = 0;
    }

    return commands_processed;
}

int register_new_client(clients_t* clients, const char* tx_path, const char* rx_path, int epoll_fd) {
    client_t* new_client = create_client(clients, tx_path, rx_path);
    if (!new_client) {
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = new_client->rx_fd;
    event.data.u32 = new_client->client_id;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client->rx_fd, &event) == -1) {
        perror("epoll_ctl");
        close_client_connection(clients, new_client->client_id, epoll_fd);
        return -1;
    }

    if (send_ack_to_client(new_client->tx_fd) != 0) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, new_client->rx_fd, NULL);
        close_client_connection(clients, new_client->client_id, epoll_fd);
        return -1;
    }

    clients->amount++;
    DEBUG_PRINTF("Client %d registered. Total: %d\n", new_client->client_id, clients->amount);
    return 0;
}

int handle_registration_command(int cmd_fifo_fd, clients_t* clients, int epoll_fd) {
    if (!clients) {
        DEBUG_PRINTF("ERROR: registration failed: null ptr\n");
        return -1;
    }

    if (clients->amount >= MAX_CLIENTS) {
        DEBUG_PRINTF("ERROR: Maximum clients reached (%d)\n", MAX_CLIENTS);
        return -1;
    }

    char buffer[1024];
    int total_commands = 0;
    const int MAX_READ_ATTEMPTS = 100;

    for (int read_attempts = 0; read_attempts < MAX_READ_ATTEMPTS; read_attempts++) {
        ssize_t n = read(cmd_fifo_fd, buffer, sizeof(buffer) - 1);

        if (n > 0) {
            buffer[n] = '\0';

            fifo_buffer_t* buf = &clients->cmd_buffer;
            size_t available = sizeof(buf->buffer) - buf->pos - 1;

            if ((size_t)n > available) {
                buf->pos = 0;
                available = sizeof(buf->buffer) - 1;
            }

            size_t to_copy = (size_t)n < available ? (size_t)n : available;
            memcpy(buf->buffer + buf->pos, buffer, to_copy);
            buf->pos += to_copy;
            buf->buffer[buf->pos] = '\0';

            total_commands += process_registration_buffer(buf, clients, epoll_fd);

        } else if (n == 0) {
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("read from registration FIFO");
                return -1;
            }
        }
    }

    return total_commands > 0 ? 0 : -1;
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
        client->tx_fd = -1;
        return NULL;
    }

    strncpy(client->tx_path, tx_path, sizeof(client->tx_path) - 1);
    client->tx_path[sizeof(client->tx_path) - 1] = '\0';

    strncpy(client->rx_path, rx_path, sizeof(client->rx_path) - 1);
    client->rx_path[sizeof(client->rx_path) - 1] = '\0';

    client->client_id = client_id;
    client->registered = 1;
    client->cmd_len = 0;

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

int process_client_commands(client_t* client, char* buffer, ssize_t n,
                                  tx_threads_pool_t* thread_pool, int client_id) {
    char full_buffer[2048];
    size_t full_len = 0;

    if (client->cmd_len > 0) {
        memcpy(full_buffer, client->cmd_buffer, client->cmd_len);
        full_len = client->cmd_len;
        client->cmd_len = 0;
    }

    memcpy(full_buffer + full_len, buffer, n);
    full_len += n;
    full_buffer[full_len] = '\0';

    char* current_pos = full_buffer;

    while (current_pos < full_buffer + full_len) {
        char* line_end = strchr(current_pos, '\n');
        if (!line_end) {
            size_t remaining = full_buffer + full_len - current_pos;
            if (remaining > 0 && remaining < sizeof(client->cmd_buffer)) {
                memcpy(client->cmd_buffer, current_pos, remaining);
                client->cmd_len = remaining;
            }
            break;
        }

        *line_end = '\0';

        if (strcmp(current_pos, DISCONNECT_CMD) == 0) {
            DEBUG_PRINTF("Client %d sent DISCONNECT command\n", client_id);
            return 1;
        }
        else if (process_get_command(current_pos, thread_pool, client_id, client->tx_fd) != 0) {
            DEBUG_PRINTF("ERROR: Failed to process GET command\n");
        }

        current_pos = line_end + 1;
    }

    return 0;
}

int process_get_command(char* command, tx_threads_pool_t* thread_pool, int client_id, int client_tx_fd) {
    char file_path[256];
    if (sscanf(command, "GET %255s", file_path) != 1) {
        DEBUG_PRINTF("ERROR: Invalid GET command: %s\n", command);
        return -1;
    }

    DEBUG_PRINTF("Client %d requested file: %s\n", client_id, file_path);

    if (!file_exists(file_path)) {
        DEBUG_PRINTF("ERROR: File not found: %s\n", file_path);
        return -1;
    }

    struct stat st;
    if (stat(file_path, &st) == 0) {
        DEBUG_PRINTF("File size: %ld bytes\n", st.st_size);
    }

    if (tx_pool_submit_task(thread_pool, client_id, client_tx_fd, file_path) != 0) {
        DEBUG_PRINTF("ERROR: Failed to submit task for client %d\n", client_id);
        return -1;
    }

    return 0;
}

int handle_get_command(tx_threads_pool_t* thread_pool, int client_id,
                      clients_t* clients, int epoll_fd) {
    if (!clients || client_id < 0 || client_id >= MAX_CLIENTS ||
        !clients->clients[client_id].registered) {
        DEBUG_PRINTF("ERROR: invalid client id %d\n", client_id);
        return -1;
    }

    client_t* client = &clients->clients[client_id];
    char buffer[1024];

    ssize_t n = read(client->rx_fd, buffer, sizeof(buffer) - 1);

    if (n > 0) {
        buffer[n] = '\0';

        if (process_client_commands(client, buffer, n, thread_pool, client_id)) {
            close_client_connection(clients, client_id, epoll_fd);
            return 0;
        }

    } else if (n == 0) {
        DEBUG_PRINTF("Client %d disconnected\n", client_id);
        close_client_connection(clients, client_id, epoll_fd);
        return 0;
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            DEBUG_PRINTF("ERROR: Read error from client %d: ", client_id);
            perror("");
            close_client_connection(clients, client_id, epoll_fd);
            return -1;
        }
    }

    return 0;
}

int send_file_to_client(int client_tx_fd, const char* filename) {
    if (fcntl(client_tx_fd, F_GETFD) == -1) {
        DEBUG_PRINTF("ERROR: Client disconnected before file transfer: %s\n", filename);
        return -1;
    }

    FILE* file = fopen(filename, "rb");
    if (!file) {
        DEBUG_PRINTF("ERROR: Cannot open file: %s - ", filename);
        perror("");
        return -1;
    }

    struct stat st;
    if (stat(filename, &st) != 0) {
        DEBUG_PRINTF("ERROR: Cannot get file size: %s - ", filename);
        perror("");
        fclose(file);
        return -1;
    }

    off_t file_size = st.st_size;
    DEBUG_PRINTF("Sending file: %s (%ld bytes)\n", filename, file_size);

    char size_header[64];
    snprintf(size_header, sizeof(size_header), "SIZE:%ld\n", file_size);
    int header_len = strlen(size_header);

    ssize_t header_sent = write(client_tx_fd, size_header, header_len);

    if (header_sent != header_len) {
        if (header_sent == -1 && errno == EPIPE) {
            DEBUG_PRINTF("ERROR: Client closed connection while sending header\n");
        } else {
            DEBUG_PRINTF("ERROR: Failed to send size header\n");
        }
        fclose(file);
        return -1;
    }

    char buffer[4096];
    size_t total_bytes_sent = 0;

    while (total_bytes_sent < (size_t)file_size) {
        size_t bytes_to_read = sizeof(buffer);
        if ((size_t)file_size - total_bytes_sent < bytes_to_read) {
            bytes_to_read = file_size - total_bytes_sent;
        }

        size_t bytes_read = fread(buffer, 1, bytes_to_read, file);

        if (bytes_read == 0) {
            if (ferror(file)) {
                DEBUG_PRINTF("ERROR: File read error: %s\n", filename);
                fclose(file);
                return -1;
            }
            break;
        }

        size_t bytes_sent = 0;
        while (bytes_sent < bytes_read) {
            ssize_t result = write(client_tx_fd, buffer + bytes_sent, bytes_read - bytes_sent);

            if (result == -1) {
                if (errno == EINTR) continue;

                if (errno == EPIPE) {
                    DEBUG_PRINTF("ERROR: Client closed connection during file transfer\n");
                } else {
                    DEBUG_PRINTF("ERROR: Write error to client fd: %s\n", strerror(errno));
                }
                fclose(file);
                return -1;
            }

            if (result == 0) {
                DEBUG_PRINTF("ERROR: FIFO closed by client\n");
                fclose(file);
                return -1;
            }

            bytes_sent += result;
            total_bytes_sent += result;
        }
    }

    fclose(file);

    if (total_bytes_sent != (size_t)file_size) {
        DEBUG_PRINTF("ERROR: Size mismatch: sent %zu, expected %ld\n", total_bytes_sent, file_size);
        return -1;
    }

    DEBUG_PRINTF("File %s sent successfully (%zu bytes)\n", filename, total_bytes_sent);
    return 0;
}

void dump_server_state(clients_t* clients, tx_threads_pool_t* pool, int epoll_fd, int cmd_fifo_fd) {
    DEBUG_PRINTF("SERVER STATE DUMP\n");

    DEBUG_PRINTF("EPOLL FD: %d, CMD_FIFO FD: %d\n", epoll_fd, cmd_fifo_fd);
    DEBUG_PRINTF("CLIENTS: total=%d, max=%d\n", clients->amount, MAX_CLIENTS);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_t* client = &clients->clients[i];
        if (client->registered) {
            DEBUG_PRINTF("Client %d: tx_fd=%d, rx_fd=%d\n", i, client->tx_fd, client->rx_fd);

            if (client->tx_fd != -1 && fcntl(client->tx_fd, F_GETFD) == -1) {
                DEBUG_PRINTF("ERROR: TX FD %d is INVALID\n", client->tx_fd);
            }
            if (client->rx_fd != -1 && fcntl(client->rx_fd, F_GETFD) == -1) {
                DEBUG_PRINTF("ERROR: RX FD %d is INVALID\n", client->rx_fd);
            }
        }
    }

    if (pool) {
        DEBUG_PRINTF("THREAD POOL: threads_count=%d\n", pool->threads_count);

        for (int i = 0; i < pool->threads_count; i++) {
            tx_thread_t* thread = &pool->threads[i];
            if (thread->is_working) {
                DEBUG_PRINTF("Thread %d: working on client %d, file: %s\n",
                           i, thread->client_id, thread->filename);
            }
        }
    }

    DEBUG_PRINTF("END SERVER STATE DUMP\n");
}

void cleanup_server_resources(int cmd_fifo_fd, int epoll_fd, tx_threads_pool_t* pool) {
    DEBUG_PRINTF("Cleaning up server resources\n");
    
    if (pool == NULL && global_pool != NULL) {
        pool = global_pool;
    }
    
    if (pool) {
        tx_pool_destroy(pool);
        global_pool = NULL;
    }
    
    if (epoll_fd == -1 && global_epoll_fd != -1) {
        epoll_fd = global_epoll_fd;
    }
    
    if (epoll_fd != -1) {
        close(epoll_fd);
        global_epoll_fd = -1;
    }
    
    if (cmd_fifo_fd == -1 && global_cmd_fifo_fd != -1) {
        cmd_fifo_fd = global_cmd_fifo_fd;
    }
    
    if (cmd_fifo_fd != -1) {
        close(cmd_fifo_fd);
        global_cmd_fifo_fd = -1;
    }
    
    if (global_clients != NULL) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (global_clients->clients[i].registered) {
                close_client_connection(global_clients, i, epoll_fd);
            }
        }
        free(global_clients);
        global_clients = NULL;
    }
    
    if (file_exists(CLIENT_SERVER_FIFO)) {
        remove_fifo_and_empty_dirs(CLIENT_SERVER_FIFO);
        DEBUG_PRINTF("Removed server FIFO: %s\n", CLIENT_SERVER_FIFO);
    }
    
    DEBUG_PRINTF("Server cleanup completed\n");
}

void close_client_connection(clients_t* clients, int client_id, int epoll_fd) {
    if (!clients || client_id < 0 || client_id >= MAX_CLIENTS || 
        !clients->clients[client_id].registered) {
        return;
    }
    
    client_t* client = &clients->clients[client_id];
    
    if (epoll_fd != -1 && client->rx_fd != -1) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->rx_fd, NULL);
    }
    
    if (client->tx_fd != -1) {
        close(client->tx_fd);
        client->tx_fd = -1;
    }
    
    if (client->rx_fd != -1) {
        close(client->rx_fd);
        client->rx_fd = -1;
    }
    
    client->cmd_len = 0;
    
    DEBUG_PRINTF("Closed connection for client %d\n", client_id);
    
    client->registered = 0;
    client->client_id = -1;
    client->tx_path[0] = '\0';
    client->rx_path[0] = '\0';
    
    int actual_count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients->clients[i].registered) {
            actual_count++;
        }
    }
    clients->amount = actual_count;
}
