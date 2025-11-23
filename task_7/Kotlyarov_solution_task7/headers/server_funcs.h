#ifndef SERVER_FUNCS
#define SERVER_FUNCS

#include <signal.h>
#include "client_server_cfg.h"
#include "fifo_utils.h"
#include "thread_utils.h"

extern int server_should_exit;

typedef struct {
    char buffer[4096];
    size_t pos;
} fifo_buffer_t;

typedef struct {
    client_t clients[MAX_EVENTS];
    int amount;
    fifo_buffer_t cmd_buffer;
} clients_t;

void run_server_loop(tx_threads_pool_t* thread_pool, int epoll_fd, int cmd_fifo_fd);
int handle_registration_command(int cmd_fifo_fd, clients_t* clients, int epoll_fd);
int send_ack_to_client(int client_tx_fd);
int handle_get_command(tx_threads_pool_t* thread_pool, int client_id, clients_t* clients);
client_t* create_client(clients_t* clients, const char* tx_path, const char* rx_path);
int find_free_client_slot(clients_t* clients);
void setup_server_signal_handlers(void);
void cleanup_server_resources(int cmd_fifo_fd, int epoll_fd, tx_threads_pool_t* pool);
void close_client_connection(clients_t* clients, int client_id);

#endif
