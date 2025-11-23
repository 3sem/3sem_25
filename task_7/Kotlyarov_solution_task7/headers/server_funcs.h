#ifndef SERVER_FUNCS
#define SERVER_FUNCS

#include "client_server_cfg.h"
#include "fifo_utils.h"
#include "thread_utils.h"

typedef struct {
    client_t clients[MAX_EVENTS];
    int amount;
} clients_t;

void run_server_loop(tx_threads_pool_t* thread_pool, int epoll_fd, int cmd_fifo_fd);
int handle_registration_command(int cmd_fifo_fd, clients_t* clients, int epoll_fd);
int send_ack_to_client(int client_tx_fd);
int handle_get_command(tx_threads_pool_t* thread_pool, int client_id, clients_t* clients);
client_t* create_client(clients_t* clients, const char* tx_path, const char* rx_path);
int find_free_client_slot(clients_t* clients);
//int send_file_to_client(int client_tx_fd, const char* filename);

#endif
