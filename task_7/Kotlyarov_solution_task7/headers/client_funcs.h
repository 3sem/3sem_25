#ifndef CLIENT_FUNCS
#define CLIENT_FUNCS

#include <signal.h>
#include <sys/select.h>
#include "fifo_utils.h"
#include "client_server_cfg.h"
#include "Debug_printf.h"

#define BUFFER_SIZE (4 * 1024 * 1024)

int register_client(client_t* client_info, int cl_s_fifo_fd);
int setup_client_fifos(const char* tx_path, const char* rx_path);
void cleanup_client_fifos(const char* tx_path, const char* rx_path);
int client_recieve_and_print(int tx_fd);
int send_get_command(client_t* client_info, const char* filename);
void signal_handler(int sig);

#endif
