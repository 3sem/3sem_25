#include "client_funcs.h"

static char global_tx_path[256] = "";
static char global_rx_path[256] = "";

void setup_client_signals(void) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGSEGV, signal_handler);
}

bool parse_client_arguments(int argc, char* argv[], file_types* files) {
    init_file_types(files);

    bool parse_success = check_r_w_flags(OPTIONAL_CHECK, argv, argc, files);

    if (!parse_success) {
        fprintf(stderr, "Error parsing arguments: %s\n", files->error_message);
        print_usage(argv[0]);
        cleanup_file_types(files);
        return false;
    }

    if (files->read_files_count == 0 && files->write_files_count == 0) {
        if (argc > 1) {
            for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                    print_usage(argv[0]);
                    cleanup_file_types(files);
                    return true;
                }
            }
        }

        printf("No files specified. Use -h for help.\n");
        cleanup_file_types(files);
        return true;
    }

    return true;
}

FILE* setup_output_redirection(file_types* files) {
    if (files->check_status & CHECK_W) {
        const char* output_filename = files->write_files[0];
        FILE* output_file = freopen(output_filename, "w", stdout);
        if (!output_file) {
            perror("freopen");
            return NULL;
        }
        return output_file;
    }
    return NULL;
}

bool initialize_client(client_t* client_info) {
    pid_t pid = getpid();
    client_info->client_id = (int)pid;

    const char* tx_path_string = "fifos/client_%d/tx";
    const char* rx_path_string = "fifos/client_%d/rx";
    snprintf(client_info->tx_path, sizeof(client_info->tx_path),
             tx_path_string, client_info->client_id);
    snprintf(client_info->rx_path, sizeof(client_info->rx_path),
             rx_path_string, client_info->client_id);

    if (setup_client_fifos(client_info->tx_path, client_info->rx_path) != 0) {
        DEBUG_PRINTF("ERROR: Failed to setup client FIFOs\n");
        return false;
    }

    return true;
}

bool register_with_server(client_t* client_info) {
    int cl_s_fifo_fd = open(CLIENT_SERVER_FIFO, O_WRONLY);
    if (cl_s_fifo_fd == -1) {
        DEBUG_PRINTF("ERROR: '%s' was not opened\n", CLIENT_SERVER_FIFO);
        return false;
    }

    if (register_client(client_info, cl_s_fifo_fd) != 0) {
        DEBUG_PRINTF("ERROR: client registration failed\n");
        close(cl_s_fifo_fd);
        return false;
    }

    close(cl_s_fifo_fd);

    client_info->tx_fd = open(client_info->tx_path, O_RDONLY | O_NONBLOCK);
    if (client_info->tx_fd == -1) {
        perror("open TX FIFO");
        return false;
    }

    client_info->rx_fd = open(client_info->rx_path, O_WRONLY);
    if (client_info->rx_fd == -1) {
        perror("open RX FIFO");
        close(client_info->tx_fd);
        return false;
    }

    return true;
}

void process_file_requests(client_t* client_info, file_types* files) {
    int success_count = 0;
    int total_files = files->read_files_count;

    if (total_files > 0) {
        for (int i = 0; i < total_files; i++) {
            const char* filename = files->read_files[i];
            if (send_get_command(client_info, filename) != 0) {
                DEBUG_PRINTF("ERROR: Failed to send GET command for: %s\n", filename);
                continue;
            }
            if (client_recieve_and_print(client_info->tx_fd) != 0) {
                DEBUG_PRINTF("ERROR: Failed to receive file: %s\n", filename);
            } else {
                success_count++;
            }
        }

        if (success_count == 0) {
            DEBUG_PRINTF("No files were received successfully\n");
        }
    } else {
        printf("No files to request. Use -r <filename> to request files.\n");
        printf("Use -h for help.\n");
    }
}

void cleanup_client_resources(client_t* client_info, file_types* files, FILE* output_file) {
    if (client_info->tx_fd != -1) {
        close(client_info->tx_fd);
    }
    if (client_info->rx_fd != -1) {
        close(client_info->rx_fd);
    }

    cleanup_client_fifos(client_info->tx_path, client_info->rx_path);
    cleanup_file_types(files);

    if (output_file) {
        fclose(output_file);
    }
}

void signal_handler(int sig) {
    DEBUG_PRINTF("Received signal %d, cleaning up\n", sig);

    if (strlen(global_rx_path) > 0) {
        int rx_fd = open(global_rx_path, O_WRONLY);
        if (rx_fd != -1) {
            write(rx_fd, DISCONNECT_CMD "\n", strlen(DISCONNECT_CMD) + 1);
            close(rx_fd);
        }
    }

    if (strlen(global_tx_path) > 0 && strlen(global_rx_path) > 0) {
        cleanup_client_fifos(global_tx_path, global_rx_path);
    }

    exit(0);
}

int setup_client_fifos(const char* tx_path, const char* rx_path) {
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
    remove_fifo_and_empty_dirs(tx_path);
    remove_fifo_and_empty_dirs(rx_path);
    global_tx_path[0] = '\0';
    global_rx_path[0] = '\0';
}

static int wait_for_ack(int temp_tx_fd, int timeout_ms) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = temp_tx_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, temp_tx_fd, &event) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        return -1;
    }

    struct epoll_event events[1];
    int nfds = epoll_wait(epoll_fd, events, 1, timeout_ms);
    
    if (nfds == -1) {
        perror("epoll_wait");
        close(epoll_fd);
        return -1;
    } else if (nfds == 0) {
        DEBUG_PRINTF("ERROR: Registration timeout\n");
        close(epoll_fd);
        return -1;
    }
    
    char response[16];
    ssize_t n = read(temp_tx_fd, response, sizeof(response) - 1);
    if (n <= 0) {
        perror("read ACK");
        close(epoll_fd);
        return -1;
    }
    
    response[n] = '\0';
    close(epoll_fd);

    if (strncmp(response, ACKNOWLEDGE_CMD, strlen(ACKNOWLEDGE_CMD)) != 0) {
        DEBUG_PRINTF("ERROR: wrong acknowledge: %s\n", response);
        return -1;
    }

    return 0;
}

int register_client(client_t* client_info, int cl_s_fifo_fd) {
    if (!client_info) {
        DEBUG_PRINTF("ERROR: null client info\n");
        return -1;
    }

    char reg_cmd[512]; 
    int written = snprintf(reg_cmd, sizeof(reg_cmd), "REGISTER %s %s\n", 
                          client_info->tx_path, client_info->rx_path);
    
    if (written < 0 || (size_t)written >= sizeof(reg_cmd)) {
        DEBUG_PRINTF("ERROR: registration command too long\n");
        return -1;
    }

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

    if (wait_for_ack(temp_tx_fd, 5000) != 0) {
        close(temp_tx_fd);
        return -1;
    }

    close(temp_tx_fd);
    return 0;
}

static int setup_epoll(int tx_fd) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }
    
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = tx_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tx_fd, &event) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        return -1;
    }
    
    return epoll_fd;
}

int read_size_header(int tx_fd, int epoll_fd, off_t* expected_size, size_t* total_bytes_read) {
    char header_buffer[128] = {0};
    size_t header_len = 0;
    const int timeout_ms = 5000;
    struct epoll_event events[1];

    while (header_len < sizeof(header_buffer) - 1) {
        int nfds = epoll_wait(epoll_fd, events, 1, timeout_ms);
        
        if (nfds == -1) {
            perror("epoll_wait header");
            return -1;
        } else if (nfds == 0) {
            DEBUG_PRINTF("ERROR: Timeout waiting for size header\n");
            return -1;
        }
        
        ssize_t bytes_read = read(tx_fd, header_buffer + header_len, sizeof(header_buffer) - header_len - 1);
        
        if (bytes_read > 0) {
            header_len += bytes_read;
            header_buffer[header_len] = '\0';
            
            char* newline = strchr(header_buffer, '\n');
            if (newline) {
                *newline = '\0';
                
                long file_size;
                if (sscanf(header_buffer, "SIZE:%ld", &file_size) != 1) {
                    DEBUG_PRINTF("ERROR: Invalid size header: %s\n", header_buffer);
                    return -1;
                }
                *expected_size = (off_t)file_size;
                
                size_t data_offset = (newline - header_buffer) + 1;
                if (header_len > data_offset) {
                    size_t initial_data = header_len - data_offset;
                    ssize_t written = write(STDOUT_FILENO, header_buffer + data_offset, initial_data);
                    if (written != (ssize_t)initial_data) {
                        DEBUG_PRINTF("ERROR: Failed to write initial data\n");
                        return -1;
                    }
                    *total_bytes_read += initial_data;
                }
                return 0;
            }
        } else if (bytes_read == 0) {
            DEBUG_PRINTF("ERROR: Server closed connection while reading header\n");
            return -1;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read header");
            return -1;
        }
    }
    
    DEBUG_PRINTF("ERROR: Size header not found\n");
    return -1;
}

int read_file_data(int tx_fd, int epoll_fd, off_t expected_size, size_t* total_bytes_read) {
    char buffer[4096];
    const int timeout_ms = 5000;
    struct epoll_event events[1];

    while (*total_bytes_read < (size_t)expected_size) {
        int nfds = epoll_wait(epoll_fd, events, 1, timeout_ms);
        
        if (nfds == -1) {
            perror("epoll_wait data");
            return -1;
        } else if (nfds == 0) {
            DEBUG_PRINTF("ERROR: Timeout waiting for file data. Received %zu/%ld bytes\n", 
                        *total_bytes_read, expected_size);
            return -1;
        }
        
        size_t bytes_to_read = sizeof(buffer);
        size_t remaining = (size_t)expected_size - *total_bytes_read;
        if (remaining < bytes_to_read) {
            bytes_to_read = remaining;
        }
        
        ssize_t bytes_read = read(tx_fd, buffer, bytes_to_read);
        
        if (bytes_read > 0) {
            *total_bytes_read += bytes_read;
            
            ssize_t bytes_written = 0;
            while (bytes_written < bytes_read) {
                ssize_t result = write(STDOUT_FILENO, buffer + bytes_written, bytes_read - bytes_written);
                
                if (result == -1) {
                    if (errno == EINTR) continue;
                    if (errno == EPIPE) {
                        DEBUG_PRINTF("ERROR: Stdout closed\n");
                        return -1;
                    }
                    perror("write stdout");
                    return -1;
                }
                bytes_written += result;
            }
            
            fflush(stdout);
            
        } else if (bytes_read == 0) {
            DEBUG_PRINTF("ERROR: Server closed connection early. Received %zu/%ld bytes\n", 
                        *total_bytes_read, expected_size);
            return -1;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("read data");
            return -1;
        }
    }
    
    return 0;
}

int client_recieve_and_print(int tx_fd) {
    if (tx_fd < 0) {
        DEBUG_PRINTF("ERROR: invalid file descriptor\n");
        return -1;
    }

    int epoll_fd = setup_epoll(tx_fd);
    if (epoll_fd == -1) {
        return -1;
    }

    off_t expected_size = -1;
    size_t total_bytes_read = 0;

    if (read_size_header(tx_fd, epoll_fd, &expected_size, &total_bytes_read) != 0) {
        close(epoll_fd);
        return -1;
    }

    if (read_file_data(tx_fd, epoll_fd, expected_size, &total_bytes_read) != 0) {
        close(epoll_fd);
        return -1;
    }

    close(epoll_fd);

    if (total_bytes_read != (size_t)expected_size) {
        DEBUG_PRINTF("ERROR: Size mismatch: received %zu, expected %ld\n", 
                    total_bytes_read, expected_size);
        return -1;
    }
    
    return 0;
}

int send_get_command(client_t* client_info, const char* filename) {
    if (!client_info || !filename) {
        DEBUG_PRINTF("ERROR: invalid arguments\n");
        return -1;
    }

    char command[512];
    snprintf(command, sizeof(command), "GET %s\n", filename);

    ssize_t result = write(client_info->rx_fd, command, strlen(command));
    if (result <= 0) {
        if (result == -1) {
            if (errno == EPIPE) {
                DEBUG_PRINTF("ERROR: Server closed connection\n");
            } else {
                perror("write GET command");
            }
        }
        return -1;
    }

    return 0;
}

int send_disconnect_command(client_t* client_info) {
    if (!client_info) {
        DEBUG_PRINTF("ERROR: invalid client info for disconnect\n");
        return -1;
    }

    ssize_t result = write(client_info->rx_fd, DISCONNECT_CMD "\n", strlen(DISCONNECT_CMD) + 1);
    if (result <= 0) {
        if (result == -1 && errno != EPIPE) {
            perror("write DISCONNECT command");
        }
    }

    return 0;
}
